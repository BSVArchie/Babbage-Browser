package main

import (
	"encoding/hex"
	"fmt"

	ec "github.com/bsv-blockchain/go-sdk/primitives/ec"
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

	// Build transaction structure
	tx := &Transaction{
		Version:  1,
		Inputs:   make([]*TxInput, len(selectedUTXOs)),
		Outputs:  make([]*TxOutput, 0),
		LockTime: 0,
	}

	// Add inputs
	for i, utxo := range selectedUTXOs {
		tx.Inputs[i] = &TxInput{
			PreviousTxID: utxo.TxID,
			Vout:         utxo.Vout,
			ScriptSig:    "", // Will be filled during signing
			Sequence:     0xffffffff,
		}
	}

	// Add recipient output
	recipientOutput := &TxOutput{
		Value:        req.Amount,
		ScriptPubKey: tb.addressToScript(req.RecipientAddress),
	}
	tx.Outputs = append(tx.Outputs, recipientOutput)

	// Add change output if needed
	if changeAmount > 0 {
		changeOutput := &TxOutput{
			Value:        changeAmount,
			ScriptPubKey: tb.addressToScript(senderAddress),
		}
		tx.Outputs = append(tx.Outputs, changeOutput)
	}

	// Serialize transaction to hex
	rawTx, err := tx.Serialize()
	if err != nil {
		return nil, fmt.Errorf("failed to serialize transaction: %v", err)
	}

	// Generate transaction ID (hash of raw transaction)
	txid := tb.calculateTxID(rawTx)

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

	// Create private key from bytes
	// Note: This is a simplified implementation for now
	// In production, you'd use the proper BSV SDK method
	_ = privateKeyBytes // Use the private key bytes in production
	privateKey := &ec.PrivateKey{} // Placeholder - will implement proper key creation
	_ = privateKey // Use the private key in production

	// Parse transaction
	tx, err := DeserializeTransaction(rawTx)
	if err != nil {
		return nil, fmt.Errorf("failed to parse transaction: %v", err)
	}

	// Sign each input
	for i, input := range tx.Inputs {
		// Create signature hash
		sigHash, err := tb.createSignatureHash(tx, i, input.ScriptPubKey)
		if err != nil {
			return nil, fmt.Errorf("failed to create signature hash: %v", err)
		}
		_ = sigHash // Use the signature hash in production

		// Sign the hash
		// Note: This is a simplified implementation for now
		// In production, you'd use the proper BSV SDK signing method
		signature := []byte("placeholder_signature") // Placeholder signature

		// Create script signature
		scriptSig := tb.createScriptSig(signature, []byte("placeholder_pubkey"))
		input.ScriptSig = scriptSig
	}

	// Serialize signed transaction
	signedRawTx, err := tx.Serialize()
	if err != nil {
		return nil, fmt.Errorf("failed to serialize signed transaction: %v", err)
	}

	txid := tb.calculateTxID(signedRawTx)

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

func (tb *TransactionBuilder) addressToScript(address string) string {
	// Convert Bitcoin SV address to script pubkey
	// This is a simplified implementation
	// In production, you'd use proper address parsing
	return "76a914" + address + "88ac" // P2PKH script template
}

func (tb *TransactionBuilder) calculateTxID(rawTx string) string {
	// Calculate double SHA256 hash of raw transaction
	// This is a simplified implementation
	// In production, you'd use proper Bitcoin transaction hashing
	return "txid_" + rawTx[:16] // Placeholder
}

func (tb *TransactionBuilder) createSignatureHash(tx *Transaction, inputIndex int, scriptPubKey string) ([]byte, error) {
	// Create signature hash for signing
	// This is a simplified implementation
	// In production, you'd implement proper SIGHASH_ALL
	return []byte("signature_hash_placeholder"), nil
}

func (tb *TransactionBuilder) createScriptSig(signature, pubKey []byte) string {
	// Create script signature
	// This is a simplified implementation
	// In production, you'd create proper P2PKH script signature
	return hex.EncodeToString(signature) + " " + hex.EncodeToString(pubKey)
}

// Transaction structure (simplified)
type Transaction struct {
	Version  uint32
	Inputs   []*TxInput
	Outputs  []*TxOutput
	LockTime uint32
}

type TxInput struct {
	PreviousTxID string
	Vout         uint32
	ScriptSig    string
	Sequence     uint32
	ScriptPubKey string // For signing
}

type TxOutput struct {
	Value        int64
	ScriptPubKey string
}

func (tx *Transaction) Serialize() (string, error) {
	// Serialize transaction to hex
	// This is a simplified implementation
	// In production, you'd implement proper Bitcoin transaction serialization
	return "raw_transaction_hex", nil
}

func DeserializeTransaction(rawTx string) (*Transaction, error) {
	// Deserialize hex transaction
	// This is a simplified implementation
	// In production, you'd implement proper Bitcoin transaction deserialization
	return &Transaction{}, nil
}
