package main

import (
	"encoding/hex"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"os"
	"path/filepath"

	ec "github.com/bsv-blockchain/go-sdk/primitives/ec"
	"github.com/bsv-blockchain/go-sdk/script"
	"github.com/sirupsen/logrus"
)

// IdentityData represents the wallet identity structure
type IdentityData struct {
	PublicKey  string `json:"publicKey"`
	PrivateKey string `json:"privateKey"` // Will be encrypted in production
	Address    string `json:"address"`
	BackedUp   bool   `json:"backedUp"`
}

// AddressData represents a generated address
type AddressData struct {
	Address    string `json:"address"`
	PublicKey  string `json:"publicKey"`
	PrivateKey string `json:"privateKey"` // Will be encrypted in production
	Index      int    `json:"index"`
}


// Wallet represents our Bitcoin SV wallet
type Wallet struct {
	identity *IdentityData
	logger   *logrus.Logger
}

// NewWallet creates a new wallet instance
func NewWallet() *Wallet {
	logger := logrus.New()
	logger.SetLevel(logrus.InfoLevel)

	return &Wallet{
		logger: logger,
	}
}

// CreateIdentity generates a new Bitcoin SV identity
func (w *Wallet) CreateIdentity() (*IdentityData, error) {
	w.logger.Info("Creating new Bitcoin SV identity...")

	// Generate a new private key
	privateKey, err := ec.NewPrivateKey()
	if err != nil {
		return nil, fmt.Errorf("failed to generate private key: %v", err)
	}

	// Get the public key
	publicKey := privateKey.PubKey()

	// Generate Bitcoin SV address
	address, err := script.NewAddressFromPublicKey(publicKey, true) // true = mainnet
	if err != nil {
		return nil, fmt.Errorf("failed to generate address: %v", err)
	}

	identity := &IdentityData{
		PublicKey:  hex.EncodeToString(publicKey.Compressed()), // Use Compressed() for public key
		PrivateKey: hex.EncodeToString(privateKey.Serialize()), // Use Serialize() for private key
		Address:    address.AddressString, // Use AddressString field for address
		BackedUp:   false,
	}

	w.logger.Info("Identity created successfully")
	w.logger.Infof("Address: %s", identity.Address)

	return identity, nil
}

// SaveIdentity saves the identity to a JSON file
func (w *Wallet) SaveIdentity(identity *IdentityData, filePath string) error {
	w.logger.Infof("Saving identity to: %s", filePath)

	// Ensure directory exists
	dir := filepath.Dir(filePath)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return fmt.Errorf("failed to create directory: %v", err)
	}

	// Marshal to JSON
	data, err := json.MarshalIndent(identity, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal identity: %v", err)
	}

	// Write to file
	if err := os.WriteFile(filePath, data, 0600); err != nil {
		return fmt.Errorf("failed to write identity file: %v", err)
	}

	w.logger.Info("Identity saved successfully")
	return nil
}

// LoadIdentity loads the identity from a JSON file
func (w *Wallet) LoadIdentity(filePath string) (*IdentityData, error) {
	w.logger.Infof("Loading identity from: %s", filePath)

	// Check if file exists
	if _, err := os.Stat(filePath); os.IsNotExist(err) {
		return nil, fmt.Errorf("identity file does not exist")
	}

	// Read file
	data, err := os.ReadFile(filePath)
	if err != nil {
		return nil, fmt.Errorf("failed to read identity file: %v", err)
	}

	// Unmarshal JSON
	var identity IdentityData
	if err := json.Unmarshal(data, &identity); err != nil {
		return nil, fmt.Errorf("failed to unmarshal identity: %v", err)
	}

	w.logger.Info("Identity loaded successfully")
	return &identity, nil
}

// GenerateAddress creates a new Bitcoin SV address
func (w *Wallet) GenerateAddress() (*AddressData, error) {
	w.logger.Info("Generating new Bitcoin SV address...")

	// Generate a new private key
	privateKey, err := ec.NewPrivateKey()
	if err != nil {
		return nil, fmt.Errorf("failed to generate private key: %v", err)
	}

	// Get the public key
	publicKey := privateKey.PubKey()

	// Generate Bitcoin SV address
	address, err := script.NewAddressFromPublicKey(publicKey, true) // true = mainnet
	if err != nil {
		return nil, fmt.Errorf("failed to generate address: %v", err)
	}

	addressData := &AddressData{
		Address:    address.AddressString,
		PublicKey:  hex.EncodeToString(publicKey.Compressed()),
		PrivateKey: hex.EncodeToString(privateKey.Serialize()),
		Index:      0, // Frontend will handle the display index
	}

	w.logger.Info("Address generated successfully")
	w.logger.Infof("Address: %s", addressData.Address)

	return addressData, nil
}

// GetIdentityPath returns the standard identity file path
func GetIdentityPath() string {
	homeDir, _ := os.UserHomeDir()
	return filepath.Join(homeDir, "AppData", "Roaming", "BabbageBrowser", "identity.json")
}

func main() {
	fmt.Println("�� Bitcoin Browser Go Wallet Starting...")

	// Create wallet instance
	wallet := NewWallet()

	// Set up HTTP handlers
	http.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{"status": "healthy"})
	})

	http.HandleFunc("/identity/get", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}
		identityPath := GetIdentityPath()

		// Try to load existing identity
		identity, err := wallet.LoadIdentity(identityPath)
		if err != nil {
			// Create new identity if none exists
			identity, err = wallet.CreateIdentity()
			if err != nil {
				http.Error(w, fmt.Sprintf("Failed to create identity: %v", err), http.StatusInternalServerError)
				return
			}

			// Save the new identity
			if err := wallet.SaveIdentity(identity, identityPath); err != nil {
				http.Error(w, fmt.Sprintf("Failed to save identity: %v", err), http.StatusInternalServerError)
				return
			}
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(identity)
	})

	http.HandleFunc("/identity/markBackedUp", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "POST" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}
		identityPath := GetIdentityPath()

		// Load existing identity
		identity, err := wallet.LoadIdentity(identityPath)
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to load identity: %v", err), http.StatusInternalServerError)
			return
		}

		// Mark as backed up
		identity.BackedUp = true

		// Save updated identity
		if err := wallet.SaveIdentity(identity, identityPath); err != nil {
			http.Error(w, fmt.Sprintf("Failed to save identity: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]bool{"success": true})
	})

	http.HandleFunc("/address/generate", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != "GET" {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}
		// Generate new address
		address, err := wallet.GenerateAddress()
		if err != nil {
			http.Error(w, fmt.Sprintf("Failed to generate address: %v", err), http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(address)
	})

	// Start HTTP server
	port := "8080"
	fmt.Printf("🌐 Wallet daemon listening on port %s\n", port)
	fmt.Println("📋 Available endpoints:")
	fmt.Println("  GET  /health - Health check")
	fmt.Println("  GET  /identity/get - Get wallet identity")
	fmt.Println("  POST /identity/markBackedUp - Mark wallet as backed up")
	fmt.Println("  GET  /address/generate - Generate new Bitcoin address")

	log.Fatal(http.ListenAndServe(":"+port, nil))
}
