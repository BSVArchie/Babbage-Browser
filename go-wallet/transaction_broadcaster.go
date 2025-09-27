package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strings"
	"time"

	"github.com/sirupsen/logrus"
)

// TransactionBroadcaster handles broadcasting transactions to Bitcoin SV network
type TransactionBroadcaster struct {
	logger *logrus.Logger
	client *http.Client
	miners []MinerAPI
}

// MinerAPI represents a Bitcoin SV mining pool or node API
type MinerAPI struct {
	Name    string
	URL     string
	Type    string // "mining_pool", "node", or "relay"
	Timeout time.Duration
}

// NewTransactionBroadcaster creates a new transaction broadcaster
func NewTransactionBroadcaster() *TransactionBroadcaster {
	logger := logrus.New()
	logger.SetLevel(logrus.InfoLevel)

	// Initialize BSV miners and nodes
	miners := []MinerAPI{
		{
			Name:    "WhatsOnChain",
			URL:     "https://api.whatsonchain.com/v1/bsv/main/tx/raw",
			Type:    "relay",
			Timeout: 30 * time.Second,
		},
		{
			Name:    "GorillaPool",
			URL:     "https://mapi.gorillapool.io/mapi/tx",
			Type:    "mining_pool",
			Timeout: 30 * time.Second,
		},
	}

	return &TransactionBroadcaster{
		logger: logger,
		client: &http.Client{
			Timeout: 30 * time.Second,
		},
		miners: miners,
	}
}

// BroadcastTransaction broadcasts a signed transaction to multiple BSV miners
func (tb *TransactionBroadcaster) BroadcastTransaction(signedTx string) (*BroadcastResult, error) {
	tb.logger.Infof("Broadcasting transaction to %d miners", len(tb.miners))

	result := &BroadcastResult{
		TxID:    "broadcasted_txid",
		Success: false,
		Miners:  make(map[string]string),
	}

	successCount := 0
	var lastError error

	// Try broadcasting to each miner
	for _, miner := range tb.miners {
		tb.logger.Debugf("Broadcasting to %s (%s)", miner.Name, miner.Type)

		response, err := tb.broadcastToMiner(miner, signedTx)
		if err != nil {
			tb.logger.Warnf("Failed to broadcast to %s: %v", miner.Name, err)
			result.Miners[miner.Name] = "failed: " + err.Error()
			lastError = err
		} else {
			tb.logger.Infof("Successfully broadcast to %s: %s", miner.Name, response)
			result.Miners[miner.Name] = "success: " + response
			successCount++
		}
	}

	// Consider broadcast successful if at least one miner accepted it
	if successCount > 0 {
		result.Success = true
		result.TxID = tb.extractTxID(signedTx)
		tb.logger.Infof("Transaction broadcast successful to %d/%d miners", successCount, len(tb.miners))
	} else {
		result.Success = false
		result.Error = fmt.Sprintf("all miners failed, last error: %v", lastError)
		tb.logger.Errorf("Transaction broadcast failed to all miners")
	}

	return result, nil
}

// broadcastToMiner sends transaction to a specific miner
func (tb *TransactionBroadcaster) broadcastToMiner(miner MinerAPI, signedTx string) (string, error) {
	// Create request payload based on miner type
	payload, err := tb.createPayload(miner, signedTx)
	if err != nil {
		return "", fmt.Errorf("failed to create payload: %v", err)
	}

	// Create HTTP request
	req, err := http.NewRequest("POST", miner.URL, bytes.NewBuffer(payload))
	if err != nil {
		return "", fmt.Errorf("failed to create request: %v", err)
	}

	// Set headers based on miner type
	tb.setHeaders(req, miner)

	// Set timeout for this specific miner
	client := &http.Client{Timeout: miner.Timeout}

	// Send request
	resp, err := client.Do(req)
	if err != nil {
		return "", fmt.Errorf("request failed: %v", err)
	}
	defer resp.Body.Close()

	// Read response
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", fmt.Errorf("failed to read response: %v", err)
	}

	// Debug: log the raw response
	tb.logger.Infof("Raw response from %s (status %d): %s", miner.Name, resp.StatusCode, string(body))

	// Check response status
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		return "", fmt.Errorf("miner returned status %d: %s", resp.StatusCode, string(body))
	}

	// Parse response to extract transaction ID
	txid, err := tb.parseResponse(miner, body)
	if err != nil {
		return "", fmt.Errorf("failed to parse response: %v", err)
	}

	return txid, nil
}

// createPayload creates the request payload for different miner types
func (tb *TransactionBroadcaster) createPayload(miner MinerAPI, signedTx string) ([]byte, error) {
	switch miner.Name {
	case "WhatsOnChain":
		// WhatsOnChain expects raw transaction hex in body
		return []byte(signedTx), nil

	case "GorillaPool":
		// GorillaPool mAPI expects JSON with rawtx field
		payload := map[string]string{
			"rawtx": signedTx,
		}
		return json.Marshal(payload)


	default:
		// Default: send raw transaction hex
		return []byte(signedTx), nil
	}
}

// setHeaders sets appropriate headers for different miner types
func (tb *TransactionBroadcaster) setHeaders(req *http.Request, miner MinerAPI) {
	req.Header.Set("User-Agent", "Babbage-Browser/1.0")

	switch miner.Name {
	case "WhatsOnChain":
		// WhatsOnChain expects raw hex, not JSON
		req.Header.Set("Content-Type", "text/plain")
	case "GorillaPool":
		// GorillaPool mAPI expects JSON
		req.Header.Set("Content-Type", "application/json")
		req.Header.Set("Accept", "application/json")
	default:
		// Default to text/plain for raw hex
		req.Header.Set("Content-Type", "text/plain")
	}
}

// parseResponse extracts transaction ID from miner response
func (tb *TransactionBroadcaster) parseResponse(miner MinerAPI, body []byte) (string, error) {
	switch miner.Name {
	case "WhatsOnChain":
		// WhatsOnChain returns the txid as a plain string, not JSON
		txid := string(body)
		// Remove quotes if present
		if len(txid) > 2 && txid[0] == '"' && txid[len(txid)-1] == '"' {
			txid = txid[1 : len(txid)-1]
		}
		// Remove newlines and whitespace
		txid = strings.TrimSpace(txid)
		return txid, nil

	case "GorillaPool":
		// GorillaPool mAPI returns a nested JSON structure
		var outerResponse struct {
			Payload string `json:"payload"`
		}
		if err := json.Unmarshal(body, &outerResponse); err != nil {
			return "", err
		}

		// Parse the inner payload
		var innerResponse struct {
			TxID            string `json:"txid"`
			ReturnResult    string `json:"returnResult"`
			ResultDesc      string `json:"resultDescription"`
		}
		if err := json.Unmarshal([]byte(outerResponse.Payload), &innerResponse); err != nil {
			return "", err
		}

		// Check if the transaction was successful
		if innerResponse.ReturnResult != "success" {
			return "", fmt.Errorf("GorillaPool rejected transaction: %s", innerResponse.ResultDesc)
		}

		return innerResponse.TxID, nil


	default:
		// Fallback: try to extract from raw response
		return string(body), nil
	}
}

// extractTxID extracts transaction ID from raw transaction (simplified)
func (tb *TransactionBroadcaster) extractTxID(signedTx string) string {
	// This is a simplified implementation
	// In production, you'd calculate the actual transaction hash
	return "txid_" + signedTx[:16]
}

// GetMinerStatus checks if miners are available
func (tb *TransactionBroadcaster) GetMinerStatus() map[string]bool {
	status := make(map[string]bool)

	for _, miner := range tb.miners {
		// Simple health check
		resp, err := tb.client.Get(miner.URL)
		if err != nil {
			status[miner.Name] = false
		} else {
			resp.Body.Close()
			status[miner.Name] = resp.StatusCode == 200
		}
	}

	return status
}
