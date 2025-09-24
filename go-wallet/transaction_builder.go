package main

import (
	"encoding/hex"
	"fmt"

	"github.com/bsv-blockchain/go-sdk/chainhash"
	ec "github.com/bsv-blockchain/go-sdk/primitives/ec"
	"github.com/bsv-blockchain/go-sdk/script"
	"github.com/bsv-blockchain/go-sdk/transaction"
	sighash "github.com/bsv-blockchain/go-sdk/transaction/sighash"
	"github.com/sirupsen/logrus"
)

// TransactionBuilder handles Bitcoin SV transaction creation and signing
type TransactionBuilder struct {
	wallet      *Wallet
	utxoManager *UTXOManager
	logger      *logrus.Logger
}

// NewTransactionBuilder creates a new transaction builder
func NewTransactionBuilder(wallet *Wallet) *TransactionBuilder {
	logger := logrus.New()
	logger.SetLevel(logrus.InfoLevel)

	return &TransactionBuilder{
		wallet:      wallet,
		utxoManager: NewUTXOManager(),
		logger:      logger,
	}
}

// CreateTransaction creates an unsigned Bitcoin SV transaction
func (tb *TransactionBuilder) CreateTransaction(req *TransactionRequest) (*TransactionResponse, error) {
	tb.logger.Infof("Creating transaction: %s -> %d satoshis", req.RecipientAddress, req.Amount)

	// Determine sender address
	var senderAddress string
	if req.SenderAddress != "" {
		senderAddress = req.SenderAddress
		tb.logger.Infof("Using provided sender address: %s", senderAddress)
	} else {
		// Get sender address from wallet identity
		identity, err := tb.wallet.LoadIdentity(GetIdentityPath())
		if err != nil {
			return nil, fmt.Errorf("failed to load identity: %v", err)
		}
		senderAddress = identity.Address
		tb.logger.Infof("Using wallet identity address: %s", senderAddress)
	}

	// Fetch UTXOs for sender address
	utxos, err := tb.utxoManager.FetchUTXOs(senderAddress)
	if err != nil {
		return nil, fmt.Errorf("failed to fetch UTXOs: %v", err)
	}

	// Select UTXOs for transaction
	selectedUTXOs, fee, err := tb.utxoManager.SelectUTXOs(utxos, req.Amount, req.FeeRate)
	if err != nil {
		return nil, fmt.Errorf("failed to select UTXOs: %v", err)
	}

	// Calculate change amount
	totalInput := int64(0)
	for _, utxo := range selectedUTXOs {
		totalInput += utxo.Amount
	}
	changeAmount := totalInput - req.Amount - fee

	// Build transaction structure using BSV SDK
	tx := transaction.NewTransaction()

	// Add inputs
	for _, utxo := range selectedUTXOs {
		// Convert hex string to bytes for previous transaction ID
		prevTxID, err := hex.DecodeString(utxo.TxID)
		if err != nil {
			return nil, fmt.Errorf("invalid previous transaction ID: %v", err)
		}

		// Reverse the transaction ID (Bitcoin uses little-endian)
		reverseBytes(prevTxID)

		// Create input using BSV SDK
		sourceTxID, err := chainhash.NewHash(prevTxID)
		if err != nil {
			return nil, fmt.Errorf("invalid transaction ID: %v", err)
		}
		txInput := &transaction.TransactionInput{
			SourceTXID:       sourceTxID,
			SourceTxOutIndex: utxo.Vout,
			SequenceNumber:   0xffffffff,
		}
		tx.AddInput(txInput)
	}

	// Add recipient output
	recipientScript, err := tb.addressToScript(req.RecipientAddress)
	if err != nil {
		return nil, fmt.Errorf("failed to create recipient script: %v", err)
	}
	recipientOutput := &transaction.TransactionOutput{
		Satoshis:      uint64(req.Amount),
		LockingScript: recipientScript,
	}
	tx.AddOutput(recipientOutput)

	// Add change output if needed
	if changeAmount > 0 {
		changeScript, err := tb.addressToScript(senderAddress)
		if err != nil {
			return nil, fmt.Errorf("failed to create change script: %v", err)
		}
		changeOutput := &transaction.TransactionOutput{
			Satoshis:      uint64(changeAmount),
			LockingScript: changeScript,
		}
		tx.AddOutput(changeOutput)
	}

	// Serialize transaction to hex
	rawTx := tx.Hex()

	// Generate transaction ID (hash of raw transaction)
	txid := tx.TxID().String()

	tb.logger.Infof("Transaction created successfully: %s", txid)

	return &TransactionResponse{
		TxID:        txid,
		RawTx:       rawTx,
		Fee:         fee,
		Status:      "created",
		Broadcasted: false,
	}, nil
}

// SignTransaction signs a transaction with the wallet's private key
func (tb *TransactionBuilder) SignTransaction(rawTx string, privateKeyHex string) (*TransactionResponse, error) {
	tb.logger.Info("Signing transaction")

	// Parse private key
	privateKeyBytes, err := hex.DecodeString(privateKeyHex)
	if err != nil {
		return nil, fmt.Errorf("invalid private key: %v", err)
	}

	// Create private key from bytes using BSV SDK
	privateKey, publicKey := ec.PrivateKeyFromBytes(privateKeyBytes)

	// Parse transaction from hex
	tx, err := transaction.NewTransactionFromHex(rawTx)
	if err != nil {
		return nil, fmt.Errorf("failed to parse transaction: %v", err)
	}

	// Sign each input
	for i := 0; i < tx.InputCount(); i++ {
		// Get the input
		txInput := tx.InputIdx(i)

		// Get the previous output script (we need to fetch this from UTXO data)
		_, err := tb.getPreviousOutputScript(txInput.SourceTXID.CloneBytes(), txInput.SourceTxOutIndex)
		if err != nil {
			return nil, fmt.Errorf("failed to get previous output script: %v", err)
		}

		// Create signature hash for this input
		sigHash, err := tx.CalcInputSignatureHash(uint32(i), sighash.All)
		if err != nil {
			return nil, fmt.Errorf("failed to create signature hash: %v", err)
		}

		// Sign the hash
		signature, err := privateKey.Sign(sigHash)
		if err != nil {
			return nil, fmt.Errorf("failed to sign transaction: %v", err)
		}

		// Create script signature (signature + public key)
		scriptSig := script.NewFromBytes([]byte{})
		scriptSig.AppendPushData(signature.Serialize())
		scriptSig.AppendPushData(publicKey.Compressed())

		// Set the script signature
		txInput.UnlockingScript = scriptSig
	}

	// Serialize signed transaction
	signedRawTx := tx.Hex()

	txid := tx.TxID().String()

	tb.logger.Infof("Transaction signed successfully: %s", txid)

	return &TransactionResponse{
		TxID:        txid,
		RawTx:       signedRawTx,
		Fee:         0, // Fee was calculated during creation
		Status:      "signed",
		Broadcasted: false,
	}, nil
}

// Helper methods

func (tb *TransactionBuilder) addressToScript(address string) (*script.Script, error) {
	// Convert Bitcoin SV address to script pubkey using BSV SDK
	addr, err := script.NewAddressFromString(address)
	if err != nil {
		return nil, fmt.Errorf("failed to parse address %s: %v", address, err)
	}

	// Create P2PKH script from address
	// P2PKH script: OP_DUP OP_HASH160 <pubKeyHash> OP_EQUALVERIFY OP_CHECKSIG
	scriptBytes := []byte{0x76, 0xa9, 0x14} // OP_DUP OP_HASH160 OP_PUSHDATA20
	scriptBytes = append(scriptBytes, addr.PublicKeyHash...)
	scriptBytes = append(scriptBytes, 0x88, 0xac) // OP_EQUALVERIFY OP_CHECKSIG

	return script.NewFromBytes(scriptBytes), nil
}

// Helper function to reverse byte slice (Bitcoin uses little-endian for transaction IDs)
func reverseBytes(data []byte) {
	for i, j := 0, len(data)-1; i < j; i, j = i+1, j-1 {
		data[i], data[j] = data[j], data[i]
	}
}

// getPreviousOutputScript retrieves the script pubkey for a previous transaction output
func (tb *TransactionBuilder) getPreviousOutputScript(txID []byte, vout uint32) (*script.Script, error) {
	// Convert txID back to hex string for UTXO lookup
	txIDHex := hex.EncodeToString(txID)

	// Fetch UTXOs to find the script pubkey for this output
	// This is a simplified implementation - in production, you'd cache this data
	utxos, err := tb.utxoManager.FetchUTXOs("") // We need to pass the address, but we don't have it here
	if err != nil {
		return nil, fmt.Errorf("failed to fetch UTXOs: %v", err)
	}

	// Find the matching UTXO
	for _, utxo := range utxos {
		if utxo.TxID == txIDHex && utxo.Vout == vout {
			// Convert the script pubkey from hex to script object
			scriptBytes, err := hex.DecodeString(utxo.Script)
			if err != nil {
				return nil, fmt.Errorf("invalid script pubkey: %v", err)
			}
			return script.NewFromBytes(scriptBytes), nil
		}
	}

	return nil, fmt.Errorf("previous output not found: %s:%d", txIDHex, vout)
}
