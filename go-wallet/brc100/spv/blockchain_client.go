package spv

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"

	"github.com/bsv-blockchain/go-sdk/chainhash"
	"github.com/bsv-blockchain/go-sdk/transaction"
	"github.com/sirupsen/logrus"
)

// BlockchainAPIClient handles real blockchain API calls for SPV verification
type BlockchainAPIClient struct {
	whatsOnChainAPI string
	gorillaPoolAPI   string
	httpClient       *http.Client
	logger           *logrus.Logger
}

// WhatsOnChainResponse represents the response from WhatsOnChain API
type WhatsOnChainResponse struct {
	TxID         string   `json:"txid"`
	Hash         string   `json:"hash"`
	BlockHash    string   `json:"blockhash"`
	BlockHeight  int64    `json:"blockheight"`
	Size         int      `json:"size"`
	Fee          int64    `json:"fee"`
	Time         int64    `json:"time"`
	Confirmations int64   `json:"confirmations"`
	Inputs       []Input  `json:"vin"`
	Outputs      []Output `json:"vout"`
}

// Input represents a transaction input
type Input struct {
	PrevOut struct {
		Hash  string `json:"hash"`
		Index int    `json:"n"`
	} `json:"prev_out"`
	ScriptSig string `json:"script"`
}

// Output represents a transaction output
type Output struct {
	Value    int64  `json:"value"`
	N        int    `json:"n"`
	ScriptPubKey ScriptPubKey `json:"scriptPubKey"`
}

// ScriptPubKey represents the script public key
type ScriptPubKey struct {
	Asm        string   `json:"asm"`
	Hex        string   `json:"hex"`
	ReqSigs    int      `json:"reqSigs"`
	Type       string   `json:"type"`
	Addresses  []string `json:"addresses"`
}

// GorillaPoolResponse represents the response from GorillaPool API
type GorillaPoolResponse struct {
	TxID        string `json:"txid"`
	Hash        string `json:"hash"`
	BlockHash   string `json:"blockhash"`
	BlockHeight int64  `json:"blockheight"`
	Size        int    `json:"size"`
	Fee         int64  `json:"fee"`
	Time        int64  `json:"time"`
	Confirmed   bool   `json:"confirmed"`
}

// MerkleProofResponse represents a Merkle proof from blockchain APIs
type MerkleProofResponse struct {
	BlockHeight int64    `json:"block_height"`
	MerklePath  []string `json:"merkle_path"`
	Position    int      `json:"position"`
	TxID        string   `json:"txid"`
}

// NewBlockchainAPIClient creates a new blockchain API client
func NewBlockchainAPIClient() *BlockchainAPIClient {
	return &BlockchainAPIClient{
		whatsOnChainAPI: "https://api.whatsonchain.com/v1/bsv/main",
		gorillaPoolAPI:  "https://api.gorillapool.io",
		httpClient: &http.Client{
			Timeout: 30 * time.Second,
		},
		logger: logrus.New(),
	}
}

// FetchTransactionFromBlockchain fetches a transaction from the blockchain
func (bc *BlockchainAPIClient) FetchTransactionFromBlockchain(txID string) (*WhatsOnChainResponse, error) {
	bc.logger.WithField("txID", txID).Info("Fetching transaction from blockchain")

	// Try WhatsOnChain first
	url := fmt.Sprintf("%s/tx/%s", bc.whatsOnChainAPI, txID)

	resp, err := bc.httpClient.Get(url)
	if err != nil {
		return nil, fmt.Errorf("failed to fetch transaction: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("failed to fetch transaction: HTTP %d", resp.StatusCode)
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("failed to read response body: %v", err)
	}

	var txResponse WhatsOnChainResponse
	if err := json.Unmarshal(body, &txResponse); err != nil {
		return nil, fmt.Errorf("failed to parse transaction response: %v", err)
	}

	bc.logger.WithFields(logrus.Fields{
		"txID":         txResponse.TxID,
		"blockHeight":  txResponse.BlockHeight,
		"confirmations": txResponse.Confirmations,
	}).Info("Transaction fetched successfully")

	return &txResponse, nil
}

// GetMerkleProofFromBlockchain fetches a Merkle proof from the blockchain
func (bc *BlockchainAPIClient) GetMerkleProofFromBlockchain(txID string, blockHeight int64) (*MerkleProofResponse, error) {
	bc.logger.WithFields(logrus.Fields{
		"txID":        txID,
		"blockHeight": blockHeight,
	}).Info("Fetching Merkle proof from blockchain")

	// Use GorillaPool for Merkle proofs (they have a good API for this)
	url := fmt.Sprintf("%s/merkle-proof/%s/%d", bc.gorillaPoolAPI, txID, blockHeight)

	resp, err := bc.httpClient.Get(url)
	if err != nil {
		return nil, fmt.Errorf("failed to fetch Merkle proof: %v", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("failed to fetch Merkle proof: HTTP %d", resp.StatusCode)
	}

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("failed to read Merkle proof response: %v", err)
	}

	var proofResponse MerkleProofResponse
	if err := json.Unmarshal(body, &proofResponse); err != nil {
		return nil, fmt.Errorf("failed to parse Merkle proof response: %v", err)
	}

	bc.logger.WithFields(logrus.Fields{
		"txID":        proofResponse.TxID,
		"blockHeight": proofResponse.BlockHeight,
		"pathLength":  len(proofResponse.MerklePath),
	}).Info("Merkle proof fetched successfully")

	return &proofResponse, nil
}

// VerifyTransactionConfirmation checks if a transaction is confirmed on the blockchain
func (bc *BlockchainAPIClient) VerifyTransactionConfirmation(txID string) (bool, int64, error) {
	bc.logger.WithField("txID", txID).Info("Verifying transaction confirmation")

	txResponse, err := bc.FetchTransactionFromBlockchain(txID)
	if err != nil {
		return false, 0, err
	}

	confirmed := txResponse.Confirmations > 0 && txResponse.BlockHeight > 0

	bc.logger.WithFields(logrus.Fields{
		"txID":        txID,
		"confirmed":   confirmed,
		"blockHeight": txResponse.BlockHeight,
	}).Info("Transaction confirmation verified")

	return confirmed, txResponse.BlockHeight, nil
}

// ConvertToSDKMerklePath converts API response to SDK's MerklePath structure
func (bc *BlockchainAPIClient) ConvertToSDKMerklePath(proof *MerkleProofResponse) (*transaction.MerklePath, error) {
	bc.logger.WithField("txID", proof.TxID).Info("Converting Merkle proof to SDK format")

	// Convert string hashes to chainhash.Hash
	var path [][]*transaction.PathElement
	for i, hashStr := range proof.MerklePath {
		hash, err := chainhash.NewHashFromHex(hashStr)
		if err != nil {
			return nil, fmt.Errorf("failed to parse hash %s: %v", hashStr, err)
		}

		// Create path element
		element := &transaction.PathElement{
			Offset: uint64(i),
			Hash:   hash,
		}

		path = append(path, []*transaction.PathElement{element})
	}

	sdkMerklePath := &transaction.MerklePath{
		BlockHeight: uint32(proof.BlockHeight),
		Path:        path,
	}

	bc.logger.WithFields(logrus.Fields{
		"txID":        proof.TxID,
		"blockHeight": proof.BlockHeight,
		"pathLength":  len(path),
	}).Info("Merkle proof converted to SDK format")

	return sdkMerklePath, nil
}

// ExtractIdentityDataFromTransaction extracts BRC-100 identity data from transaction outputs
func (bc *BlockchainAPIClient) ExtractIdentityDataFromTransaction(tx *WhatsOnChainResponse) (map[string]interface{}, error) {
	bc.logger.WithField("txID", tx.TxID).Info("Extracting identity data from transaction")

	identityData := make(map[string]interface{})

	// Look for BRC-100 identity data in transaction outputs
	for i, output := range tx.Outputs {
		// Check if output contains BRC-100 identity data
		// This would typically be in OP_RETURN outputs or specific script patterns
		if output.ScriptPubKey.Hex != "" {
			// In a real implementation, we would parse the script to extract BRC-100 data
			// For now, we'll create a placeholder structure
			identityData[fmt.Sprintf("output_%d", i)] = map[string]interface{}{
				"addresses": output.ScriptPubKey.Addresses,
				"value":     output.Value,
				"script":    output.ScriptPubKey.Hex,
				"type":      output.ScriptPubKey.Type,
			}
		}
	}

	// Add transaction metadata
	identityData["transaction"] = map[string]interface{}{
		"txID":          tx.TxID,
		"blockHeight":   tx.BlockHeight,
		"timestamp":     time.Unix(tx.Time, 0),
		"confirmations": tx.Confirmations,
	}

	bc.logger.WithFields(logrus.Fields{
		"txID":          tx.TxID,
		"outputsCount":  len(tx.Outputs),
		"identityFields": len(identityData),
	}).Info("Identity data extracted from transaction")

	return identityData, nil
}
