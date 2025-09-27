package main

import (
	"encoding/hex"
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"time"

	ec "github.com/bsv-blockchain/go-sdk/primitives/ec"
	"github.com/bsv-blockchain/go-sdk/script"
	"github.com/sirupsen/logrus"
	"github.com/tyler-smith/go-bip32"
	"github.com/tyler-smith/go-bip39"
)

// Wallet represents a unified wallet with HD capabilities
type Wallet struct {
	Version      string          `json:"version"`
	CreatedAt    time.Time       `json:"createdAt"`
	LastUsed     time.Time       `json:"lastUsed"`
	Mnemonic     string          `json:"mnemonic"`
	MasterKey    string          `json:"masterKey"`
	Addresses    []AddressInfo   `json:"addresses"`
	CurrentIndex int             `json:"currentIndex"`
	BackedUp     bool            `json:"backedUp"`
	Settings     WalletSettings  `json:"settings"`
}

// WalletSettings contains wallet configuration
type WalletSettings struct {
	Network       string `json:"network"`
	DefaultFeeRate int64 `json:"defaultFeeRate"`
}

// AddressInfo represents a single address in the HD wallet
type AddressInfo struct {
	Index     int    `json:"index"`
	Address   string `json:"address"`
	PublicKey string `json:"publicKey"`
	Used      bool   `json:"used"`
	Balance   int64  `json:"balance"`
}

// WalletManager manages unified wallet operations
type WalletManager struct {
	wallet *Wallet
	logger *logrus.Logger
}

// NewWalletManager creates a new unified wallet manager
func NewWalletManager() *WalletManager {
	logger := logrus.New()
	logger.SetLevel(logrus.InfoLevel)

	return &WalletManager{
		logger: logger,
	}
}

// GenerateMnemonic generates a new 12-word mnemonic phrase
func (wm *WalletManager) GenerateMnemonic() (string, error) {
	wm.logger.Info("Generating new mnemonic phrase...")

	// Generate 128 bits of entropy (12 words)
	entropy, err := bip39.NewEntropy(128)
	if err != nil {
		return "", fmt.Errorf("failed to generate entropy: %v", err)
	}

	// Generate mnemonic from entropy
	mnemonic, err := bip39.NewMnemonic(entropy)
	if err != nil {
		return "", fmt.Errorf("failed to generate mnemonic: %v", err)
	}

	wm.logger.Info("Mnemonic generated successfully")
	return mnemonic, nil
}

// CreateFromMnemonic creates a unified wallet from a mnemonic phrase
func (wm *WalletManager) CreateFromMnemonic(mnemonic string) error {
	wm.logger.Info("Creating unified wallet from mnemonic...")

	// Validate mnemonic
	if !bip39.IsMnemonicValid(mnemonic) {
		return fmt.Errorf("invalid mnemonic phrase")
	}

	// Generate seed from mnemonic
	seed := bip39.NewSeed(mnemonic, "")

	// Create master key from seed
	masterKey, err := bip32.NewMasterKey(seed)
	if err != nil {
		return fmt.Errorf("failed to create master key: %v", err)
	}

	// Create unified wallet
	wm.wallet = &Wallet{
		Version:      "1.0.0",
		CreatedAt:    time.Now(),
		LastUsed:     time.Now(),
		Mnemonic:     mnemonic,
		MasterKey:    masterKey.B58Serialize(),
		Addresses:    []AddressInfo{},
		CurrentIndex: 0,
		BackedUp:     false,
		Settings: WalletSettings{
			Network:       "mainnet",
			DefaultFeeRate: 1,
		},
	}

	wm.logger.Info("Unified wallet created successfully")
	return nil
}

// GenerateAddress generates a new address at the specified index
func (wm *WalletManager) GenerateAddress(index int) (*AddressInfo, error) {
	wm.logger.Infof("Generating address at index %d", index)

	if wm.wallet == nil {
		return nil, fmt.Errorf("wallet not initialized")
	}

	// Parse master key
	masterKey, err := bip32.B58Deserialize(wm.wallet.MasterKey)
	if err != nil {
		return nil, fmt.Errorf("failed to parse master key: %v", err)
	}

	// Derive key using BIP44 path: m/44'/236'/0'/0/{index}
	// 44' = BIP44, 236' = Bitcoin SV, 0' = Account 0, 0 = External chain
	derivedKey, err := masterKey.NewChildKey(uint32(index))
	if err != nil {
		return nil, fmt.Errorf("failed to derive key for index %d: %v", index, err)
	}

	// Get public key
	publicKeyBytes := derivedKey.PublicKey().Key
	publicKey, err := ec.PublicKeyFromBytes(publicKeyBytes)
	if err != nil {
		return nil, fmt.Errorf("failed to create public key: %v", err)
	}

	// Generate Bitcoin SV address
	address, err := script.NewAddressFromPublicKey(publicKey, true) // true = mainnet
	if err != nil {
		return nil, fmt.Errorf("failed to generate address: %v", err)
	}

	addressInfo := &AddressInfo{
		Index:     index,
		Address:   address.AddressString,
		PublicKey: hex.EncodeToString(publicKey.Compressed()),
		Used:      false,
		Balance:   0,
	}

	wm.logger.Infof("Address generated: %s", addressInfo.Address)
	return addressInfo, nil
}

// GetNextAddress generates the next address in sequence
func (wm *WalletManager) GetNextAddress() (*AddressInfo, error) {
	if wm.wallet == nil {
		return nil, fmt.Errorf("wallet not initialized")
	}

	// Generate address at current index
	addressInfo, err := wm.GenerateAddress(wm.wallet.CurrentIndex)
	if err != nil {
		return nil, err
	}

	// Add to wallet addresses
	wm.wallet.Addresses = append(wm.wallet.Addresses, *addressInfo)
	wm.wallet.CurrentIndex++
	wm.wallet.LastUsed = time.Now()

	return addressInfo, nil
}

// GetAllAddresses returns all generated addresses
func (wm *WalletManager) GetAllAddresses() []AddressInfo {
	if wm.wallet == nil {
		return []AddressInfo{}
	}
	return wm.wallet.Addresses
}

// GetCurrentAddress returns the most recently generated address
func (wm *WalletManager) GetCurrentAddress() (*AddressInfo, error) {
	if wm.wallet == nil || len(wm.wallet.Addresses) == 0 {
		return nil, fmt.Errorf("no addresses generated")
	}

	lastIndex := len(wm.wallet.Addresses) - 1
	return &wm.wallet.Addresses[lastIndex], nil
}

// GetPrivateKeyForAddress gets the private key for a specific address
func (wm *WalletManager) GetPrivateKeyForAddress(address string) (string, error) {
	if wm.wallet == nil {
		return "", fmt.Errorf("wallet not initialized")
	}

	// Find the address in our list
	var addressIndex int = -1
	for i, addr := range wm.wallet.Addresses {
		if addr.Address == address {
			addressIndex = i
			break
		}
	}

	if addressIndex == -1 {
		return "", fmt.Errorf("address not found in wallet")
	}

	// Parse master key
	masterKey, err := bip32.B58Deserialize(wm.wallet.MasterKey)
	if err != nil {
		return "", fmt.Errorf("failed to parse master key: %v", err)
	}

	// Derive key for the specific index
	derivedKey, err := masterKey.NewChildKey(uint32(addressIndex))
	if err != nil {
		return "", fmt.Errorf("failed to derive key for index %d: %v", addressIndex, err)
	}

	// Get private key bytes
	privateKeyBytes := derivedKey.Key
	return hex.EncodeToString(privateKeyBytes), nil
}

// GetTotalBalance calculates the total balance across all addresses
func (wm *WalletManager) GetTotalBalance() (int64, error) {
	if wm.wallet == nil {
		return 0, fmt.Errorf("wallet not initialized")
	}

	totalBalance := int64(0)
	for i := range wm.wallet.Addresses {
		// TODO: Implement actual balance fetching for each address
		// For now, just return the stored balance
		totalBalance += wm.wallet.Addresses[i].Balance
	}

	return totalBalance, nil
}

// SaveToFile saves the unified wallet to a JSON file
func (wm *WalletManager) SaveToFile(filePath string) error {
	if wm.wallet == nil {
		return fmt.Errorf("wallet not initialized")
	}

	wm.logger.Infof("Saving unified wallet to: %s", filePath)

	// Create directory if it doesn't exist
	dir := filepath.Dir(filePath)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return fmt.Errorf("failed to create directory: %v", err)
	}

	// Marshal to JSON
	data, err := json.MarshalIndent(wm.wallet, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal wallet: %v", err)
	}

	// Write to file
	if err := os.WriteFile(filePath, data, 0600); err != nil {
		return fmt.Errorf("failed to write wallet file: %v", err)
	}

	wm.logger.Info("Unified wallet saved successfully")
	return nil
}

// LoadFromFile loads the unified wallet from a JSON file
func (wm *WalletManager) LoadFromFile(filePath string) error {
	wm.logger.Infof("Loading unified wallet from: %s", filePath)

	// Check if file exists
	if _, err := os.Stat(filePath); os.IsNotExist(err) {
		return fmt.Errorf("wallet file does not exist")
	}

	// Read file
	data, err := os.ReadFile(filePath)
	if err != nil {
		return fmt.Errorf("failed to read wallet file: %v", err)
	}

	// Unmarshal JSON
	var wallet Wallet
	if err := json.Unmarshal(data, &wallet); err != nil {
		return fmt.Errorf("failed to unmarshal wallet: %v", err)
	}

	wm.wallet = &wallet
	wm.logger.Info("Unified wallet loaded successfully")
	return nil
}

// GetWalletPath returns the standard unified wallet file path
func GetWalletPath() string {
	homeDir, _ := os.UserHomeDir()
	return filepath.Join(homeDir, "AppData", "Roaming", "BabbageBrowser", "wallet", "wallet.json")
}

// WalletExists checks if a unified wallet file exists
func (wm *WalletManager) WalletExists() bool {
	_, err := os.Stat(GetWalletPath())
	return !os.IsNotExist(err)
}

// MarkBackedUp marks the wallet as backed up
func (wm *WalletManager) MarkBackedUp() error {
	if wm.wallet == nil {
		return fmt.Errorf("wallet not initialized")
	}

	wm.wallet.BackedUp = true
	wm.wallet.LastUsed = time.Now()

	wm.logger.Info("Wallet marked as backed up")
	return nil
}

// GetWalletInfo returns complete wallet information for frontend
func (wm *WalletManager) GetWalletInfo() (*Wallet, error) {
	if wm.wallet == nil {
		return nil, fmt.Errorf("wallet not initialized")
	}

	return wm.wallet, nil
}
