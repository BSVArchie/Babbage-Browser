package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"

	"github.com/bsv-blockchain/go-sdk/transaction"
	"github.com/sirupsen/logrus"
)

// IdentityData removed - replaced with HD wallet system

// AddressData represents a generated address
type AddressData struct {
	Address   string `json:"address"`
	PublicKey string `json:"publicKey"`
	Index     int    `json:"index"`
	// Note: Private key is NOT included for security reasons
}

// TransactionRequest represents a transaction creation request
type TransactionRequest struct {
	SenderAddress    string `json:"senderAddress,omitempty"`    // Optional: if not provided, uses wallet identity
	RecipientAddress string `json:"recipientAddress"`
	Amount          int64  `json:"amount"` // in satoshis
	FeeRate         int64  `json:"feeRate"` // satoshis per byte
	Message         string `json:"message,omitempty"` // Optional OP_RETURN message
}

// UTXO represents an unspent transaction output
type UTXO struct {
	TxID    string `json:"txid"`
	Vout    uint32 `json:"vout"`
	Amount  int64  `json:"amount"`
	Script  string `json:"script"`
	Address string `json:"address"` // Which address owns this UTXO
}

// TransactionResponse represents a transaction operation response
type TransactionResponse struct {
	TxID        string `json:"txid"`
	RawTx       string `json:"rawTx"`
	Fee         int64  `json:"fee"`
	Status      string `json:"status"`
	Broadcasted bool   `json:"broadcasted"`
}

// BroadcastResult represents the result of broadcasting to multiple miners
type BroadcastResult struct {
	TxID       string            `json:"txid"`
	Success    bool              `json:"success"`
	Miners     map[string]string `json:"miners"` // miner name -> response
	Error      string            `json:"error,omitempty"`
}


// WalletService represents our Bitcoin SV wallet service
type WalletService struct {
	walletManager      *WalletManager
	logger             *logrus.Logger
	transactionBuilder *TransactionBuilder
	broadcaster        *TransactionBroadcaster
	selectedUTXOs      []UTXO // Store selected UTXOs for signing
	createdTransaction *transaction.Transaction // Store created transaction object for signing
}

// NewWalletService creates a new wallet service instance
func NewWalletService() *WalletService {
	logger := logrus.New()
	logger.SetLevel(logrus.InfoLevel)

	walletService := &WalletService{
		walletManager: NewWalletManager(),
		logger:        logger,
	}

	// Initialize transaction components
	walletService.transactionBuilder = NewTransactionBuilder(walletService)
	walletService.broadcaster = NewTransactionBroadcaster()

	return walletService
}


func main() {
	fmt.Println("�� Bitcoin Browser Go Wallet Starting...")

	// Create wallet service instance
	walletService := NewWalletService()

	// Try to load existing wallet on startup
	if walletService.walletManager.WalletExists() {
		fmt.Println("📁 Loading existing wallet...")
		err := walletService.walletManager.LoadFromFile(GetWalletPath())
		if err != nil {
			fmt.Printf("⚠️ Failed to load existing wallet: %v\n", err)
		} else {
			fmt.Println("✅ Wallet loaded successfully")
		}
	} else {
		fmt.Println("📝 No existing wallet found - will create new wallet when needed")
	}

	// Set up HTTP handlers
	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{"status": "healthy"})
	})

	// Identity endpoints removed - replaced with HD wallet system

	// Old address generation removed - replaced with HD wallet system

	// UTXO testing endpoint
	http.HandleFunc("/utxo/fetch", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		// Get address from query parameter
		address := r.URL.Query().Get("address")
		if address == "" {
			http.Error(w, "address parameter is required", http.StatusBadRequest)
			return
		}

		// Fetch UTXOs using UTXO manager
		utxos, err := walletService.transactionBuilder.utxoManager.FetchUTXOs(address)
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to fetch UTXOs: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(utxos)
	})

	// Transaction endpoints
	http.HandleFunc("/transaction/create", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		var req TransactionRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, fmt.Sprintf("Invalid request body: %v", err), http.StatusBadRequest)
			return
		}

		// Create transaction using transaction builder
		response, err := walletService.transactionBuilder.CreateTransaction(&req)
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to create transaction: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	http.HandleFunc("/transaction/sign", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		var req struct {
			RawTx string `json:"rawTx"`
		}
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, fmt.Sprintf("Invalid request body: %v", err), http.StatusBadRequest)
			return
		}

		// Use stored selected UTXOs for signing
		walletService.logger.Infof("Signing request received, checking stored UTXOs...")
		walletService.logger.Infof("Stored UTXOs count: %d", len(walletService.selectedUTXOs))

		if len(walletService.selectedUTXOs) == 0 {
			walletService.logger.Error("No selected UTXOs available for signing")
			http.Error(w, "No selected UTXOs available for signing", http.StatusBadRequest)
			return
		}

		// Log the stored UTXOs
		for i, utxo := range walletService.selectedUTXOs {
			walletService.logger.Infof("Stored UTXO %d: %s:%d (amount: %d, address: %s)", i, utxo.TxID, utxo.Vout, utxo.Amount, utxo.Address)
		}

		// Sign transaction using transaction builder with selected UTXOs
		response, err := walletService.transactionBuilder.SignTransaction(req.RawTx, "", walletService.selectedUTXOs)
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to sign transaction: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	http.HandleFunc("/transaction/broadcast", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		var req struct {
			SignedTx string `json:"signedTx"`
		}
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, fmt.Sprintf("Invalid request body: %v", err), http.StatusBadRequest)
			return
		}

		// Broadcast transaction using broadcaster
		response, err := walletService.broadcaster.BroadcastTransaction(req.SignedTx)
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to broadcast transaction: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	// Transaction history endpoint (placeholder - returns empty array for now)
	http.HandleFunc("/transaction/history", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		// For now, return empty array - in production this would query a database
		// or blockchain explorer for transaction history
		history := []map[string]interface{}{}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(history)
	})

	// Unified Wallet Management Endpoints
	http.HandleFunc("/wallet/status", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		exists := walletService.walletManager.WalletExists()
		response := map[string]interface{}{
			"exists": exists,
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	http.HandleFunc("/wallet/create", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		// Generate new mnemonic
		mnemonic, err := walletService.walletManager.GenerateMnemonic()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to generate mnemonic: %v", err), http.StatusInternalServerError)
			return
		}

		// Create unified wallet from mnemonic
		err = walletService.walletManager.CreateFromMnemonic(mnemonic)
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to create unified wallet: %v", err), http.StatusInternalServerError)
			return
		}

		// Save to file
		err = walletService.walletManager.SaveToFile(GetWalletPath())
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to save unified wallet: %v", err), http.StatusInternalServerError)
			return
		}

		response := map[string]interface{}{
			"success": true,
			"mnemonic": mnemonic,
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	http.HandleFunc("/wallet/load", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		err := walletService.walletManager.LoadFromFile(GetWalletPath())
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to load unified wallet: %v", err), http.StatusInternalServerError)
			return
		}

		response := map[string]interface{}{
			"success": true,
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	// Address Management Endpoints
	http.HandleFunc("/wallet/addresses", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		addresses := walletService.walletManager.GetAllAddresses()
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(addresses)
	})

	http.HandleFunc("/wallet/address/generate", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		address, err := walletService.walletManager.GetNextAddress()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to generate address: %v", err), http.StatusInternalServerError)
			return
		}

		// Save wallet after generating new address
		err = walletService.walletManager.SaveToFile(GetWalletPath())
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to save wallet: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(address)
	})

	http.HandleFunc("/wallet/address/current", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		address, err := walletService.walletManager.GetCurrentAddress()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to get current address: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(address)
	})

	// Balance Management Endpoints
	http.HandleFunc("/wallet/balance", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		balance, err := walletService.walletManager.GetTotalBalance()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to get balance: %v", err), http.StatusInternalServerError)
			return
		}

		response := map[string]interface{}{
			"balance": balance,
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	// New unified wallet info endpoint
	http.HandleFunc("/wallet/info", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		walletInfo, err := walletService.walletManager.GetWalletInfo()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to get wallet info: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(walletInfo)
	})

	// Mark wallet as backed up endpoint
	http.HandleFunc("/wallet/markBackedUp", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		err := walletService.walletManager.MarkBackedUp()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to mark wallet as backed up: %v", err), http.StatusInternalServerError)
			return
		}

		// Save wallet after marking as backed up
		err = walletService.walletManager.SaveToFile(GetWalletPath())
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to save wallet: %v", err), http.StatusInternalServerError)
			return
		}

		response := map[string]interface{}{
			"success": true,
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	})

	// Start HTTP server
	port := "8080"
	fmt.Printf("🌐 Wallet daemon listening on port %s\n", port)
	fmt.Println("📋 Available endpoints:")
	fmt.Println("  GET  /health - Health check")
	fmt.Println("  GET  /utxo/fetch?address=ADDRESS - Fetch UTXOs for address")
	fmt.Println("  POST /transaction/create - Create unsigned transaction")
	fmt.Println("  POST /transaction/sign - Sign transaction")
	fmt.Println("  POST /transaction/broadcast - Broadcast transaction to BSV network")
	fmt.Println("  GET  /transaction/history - Get transaction history")
	fmt.Println("  GET  /wallet/status - Check if unified wallet exists")
	fmt.Println("  POST /wallet/create - Create new unified wallet")
	fmt.Println("  POST /wallet/load - Load existing unified wallet")
	fmt.Println("  GET  /wallet/info - Get complete wallet information")
	fmt.Println("  POST /wallet/markBackedUp - Mark wallet as backed up")
	fmt.Println("  GET  /wallet/addresses - Get all addresses")
	fmt.Println("  POST /wallet/address/generate - Generate new address")
	fmt.Println("  GET  /wallet/address/current - Get current address")
	fmt.Println("  GET  /wallet/balance - Get total balance")

	log.Fatal(http.ListenAndServe(":"+port, nil))
}
