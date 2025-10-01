# API References - Bitcoin Browser

## 🎯 Overview

This document outlines all API interfaces in the Babbage Browser project, including frontend-backend communication, wallet operations, and blockchain integration.

## 📱 Frontend ↔ CEF Bridge APIs

### Identity Management
```typescript
// Wallet identity operations
window.bitcoinBrowser.identity.get(): Promise<IdentityData>
window.bitcoinBrowser.identity.create(): Promise<IdentityData>
window.bitcoinBrowser.identity.markBackedUp(): Promise<boolean>
window.bitcoinBrowser.identity.authenticate(challenge: string): Promise<AuthResponse>
```

## 🔐 BRC-100 Authentication APIs ✅ PRODUCTION READY

### Identity Certificate Management
```typescript
// Generate BRC-100 identity certificate
window.bitcoinBrowser.brc100.identity.generate(data: IdentityRequest): Promise<BRC100Response>

// Validate identity certificate
window.bitcoinBrowser.brc100.identity.validate(certificate: IdentityCertificate): Promise<BRC100Response>

// Create selective disclosure
window.bitcoinBrowser.brc100.identity.selectiveDisclosure(data: SelectiveDisclosureRequest): Promise<BRC100Response>

// Request structures
interface IdentityRequest {
  subject: string;
  attributes: Record<string, any>;
}

interface SelectiveDisclosureRequest {
  identityData: Record<string, any>;
  fields: string[];
}

interface BRC100Response {
  success: boolean;
  data?: Record<string, any>;
  error?: string;
}
```

### Authentication Flow
```typescript
// Generate authentication challenge
window.bitcoinBrowser.brc100.auth.challenge(appId: string): Promise<BRC100Response>

// Authenticate with challenge response
window.bitcoinBrowser.brc100.auth.authenticate(request: AuthRequest): Promise<BRC100Response>

// Derive Type-42 keys for P2P communication
window.bitcoinBrowser.brc100.auth.type42(keys: KeyDerivationRequest): Promise<BRC100Response>

interface AuthRequest {
  appId: string;
  challenge: string;
  response: string;
  sessionId?: string;
  identityId?: string;
}

interface KeyDerivationRequest {
  walletPublicKey: string;
  appPublicKey: string;
}
```

### Session Management
```typescript
// Create authentication session
window.bitcoinBrowser.brc100.session.create(request: SessionRequest): Promise<BRC100Response>

// Validate session
window.bitcoinBrowser.brc100.session.validate(sessionId: string): Promise<BRC100Response>

// Revoke session
window.bitcoinBrowser.brc100.session.revoke(sessionId: string): Promise<BRC100Response>

interface SessionRequest {
  identityId: string;
  appId: string;
}
```

### BEEF Transactions
```typescript
// Create BRC-100 BEEF transaction
window.bitcoinBrowser.brc100.beef.create(request: BEEFRequest): Promise<BRC100Response>

// Verify BRC-100 BEEF transaction
window.bitcoinBrowser.brc100.beef.verify(transaction: BRC100BEEFTransaction): Promise<BRC100Response>

// Convert and broadcast BEEF
window.bitcoinBrowser.brc100.beef.broadcast(transaction: BRC100BEEFTransaction): Promise<BRC100Response>

interface BEEFRequest {
  actions: BRC100Action[];
  sessionId?: string;
}
```

### SPV Verification
```typescript
// Verify identity with SPV
window.bitcoinBrowser.brc100.spv.verify(request: SPVRequest): Promise<BRC100Response>

// Create SPV identity proof
window.bitcoinBrowser.brc100.spv.proof(request: SPVRequest): Promise<BRC100Response>

interface SPVRequest {
  transactionId: string;
  identityData: Record<string, any>;
}
```

### Service Status
```typescript
// Get BRC-100 service status
window.bitcoinBrowser.brc100.status(): Promise<BRC100Response>
```

### Transaction Management ✅ PRODUCTION READY
```typescript
// Unified transaction operations (create + sign + broadcast)
window.bitcoinAPI.sendTransaction(data: TransactionData): Promise<TransactionResponse>

// Transaction data structure
interface TransactionData {
  toAddress: string;
  amount: number;        // in satoshis
  feeRate: number;       // sat/byte
}

// Transaction response structure
interface TransactionResponse {
  success: boolean;
  txid: string;
  message: string;
  whatsOnChainUrl: string;
}

// Legacy transaction operations (DEPRECATED - use sendTransaction instead)
window.bitcoinBrowser.transactions.create(txData: TransactionData): Promise<Transaction>
window.bitcoinBrowser.transactions.sign(txId: string): Promise<Signature>
window.bitcoinBrowser.transactions.broadcast(tx: Transaction): Promise<BroadcastResult>
window.bitcoinBrowser.transactions.getHistory(): Promise<Transaction[]>
```

### Balance & UTXO Management ✅ PRODUCTION READY
```typescript
// Balance operations (total across all addresses)
window.bitcoinAPI.getBalance(): Promise<BalanceResponse>

// Balance response structure
interface BalanceResponse {
  balance: number;        // total balance in satoshis
  usdValue?: number;      // USD equivalent (if price available)
}

// Address operations
window.bitcoinBrowser.address.generate(): Promise<AddressData>
window.bitcoinBrowser.address.getCurrent(): Promise<string>
window.bitcoinBrowser.address.getAll(): Promise<AddressData[]>

// Address data structure
interface AddressData {
  address: string;
  index: number;
  balance: number;        // balance for this specific address
}

// Legacy UTXO operations (DEPRECATED - use getBalance for total balance)
window.bitcoinBrowser.utxos.list(): Promise<UTXO[]>
window.bitcoinBrowser.utxos.refresh(): Promise<void>
```

### Process-Per-Overlay Management
```typescript
// Process-per-overlay operations (NEW ARCHITECTURE)
window.cefMessage.send(messageName: string, args: any[]): void

// Overlay-specific messages
window.cefMessage.send('overlay_show_settings', []): void
window.cefMessage.send('overlay_show_wallet', []): void
window.cefMessage.send('overlay_show_backup', []): void
window.cefMessage.send('overlay_close', []): void

// Legacy overlay operations (DEPRECATED - being phased out)
window.bitcoinBrowser.overlay.show(panelName: string): void
window.bitcoinBrowser.overlay.hide(): void
window.bitcoinBrowser.overlay.openPanel(panelName: string): void
```

### Navigation Control
```typescript
// Browser navigation
window.bitcoinBrowser.navigation.navigate(url: string): void
window.bitcoinBrowser.navigation.goBack(): void
window.bitcoinBrowser.navigation.goForward(): void
window.bitcoinBrowser.navigation.reload(): void
```

## 🔄 CEF Message System (UPDATED FOR PROCESS-PER-OVERLAY)

### Process Message Types
```cpp
// CEF process messages for frontend-backend communication

// Overlay Management (NEW ARCHITECTURE)
"overlay_show_settings"    // Create settings overlay in separate process
"overlay_show_wallet"      // Create wallet overlay in separate process
"overlay_show_backup"      // Create backup modal overlay in separate process
"overlay_close"           // Close current overlay window

// Identity Management (NEW SYSTEM)
"identity_status_check"        // Check if identity exists and needs backup
"identity_status_check_response" // Response with identity status
"create_identity"              // Create new identity via Go daemon
"create_identity_response"     // Response with identity data
"mark_identity_backed_up"      // Mark identity as backed up
"mark_identity_backed_up_response" // Response confirmation

// Utility Messages
"force_repaint"           // Force CEF to repaint overlay content

// Legacy Messages (DEPRECATED)
"navigate"           // Navigate to URL
"overlay_open_panel" // Open overlay panel (DEPRECATED)
"overlay_show"       // Show overlay window (DEPRECATED)
"overlay_hide"       // Hide overlay window (DEPRECATED)
"overlay_input"      // Toggle mouse input
"address_generate"   // Generate new address
"identity_get"       // Get wallet identity (DEPRECATED)
"identity_backup"    // Mark wallet as backed up (DEPRECATED)
```

### Message Response System
```typescript
// Frontend listens for responses via CustomEvent
window.addEventListener('cefMessageResponse', (event) => {
    const { message, args } = event.detail;

    switch (message) {
        case 'identity_status_check_response':
            const status = JSON.parse(args[0]);
            // Handle identity status
            break;
        case 'create_identity_response':
            const identity = JSON.parse(args[0]);
            // Handle identity creation
            break;
        case 'mark_identity_backed_up_response':
            const result = JSON.parse(args[0]);
            // Handle backup confirmation
            break;
    }
});

// API Ready Event
window.addEventListener('bitcoinBrowserReady', () => {
    // bitcoinBrowser API is fully injected and ready
    // Safe to make API calls
});
```

## 🔧 CEF ↔ Go Bridge APIs

### Wallet Operations
```cpp
// C++ to Go wallet communication
class BitcoinWalletHandler {
    // Create new wallet
    std::string createWallet(const std::string& password);

    // Load existing wallet
    std::string loadWallet(const std::string& privateKey);

    // Sign transaction
    std::string signTransaction(const std::string& txData);

    // Get wallet balance
    std::string getBalance();

    // Generate new address
    std::string generateAddress();
};
```

### BRC-100 Authentication
```cpp
// BRC-100 authentication operations
class BRC100Handler {
    // Authenticate with BRC-100
    std::string authenticate(const std::string& challenge);

    // Verify certificate
    bool verifyCertificate(const std::string& certData);

    // Create selective disclosure
    std::string selectiveDisclosure(const std::vector<std::string>& fields);
};
```

## 🐹 Go Wallet Backend APIs

### Core Wallet Functions
```go
type BitcoinWallet struct {
    password string
    // ... other fields
}

func NewBitcoinWallet(password string) *BitcoinWallet {
    // Initialize wallet with password
}

func (w *BitcoinWallet) CreateWallet() (map[string]interface{}, error) {
    // Create new wallet and return identity data
}

func (w *BitcoinWallet) LoadWallet(privateKey string) (map[string]interface{}, error) {
    // Load existing wallet from private key
}

func (w *BitcoinWallet) SignTransaction(txData map[string]interface{}) (string, error) {
    // Sign transaction using bitcoin-sv/go-sdk
}

func (w *BitcoinWallet) GetBalance() (int64, error) {
    // Get wallet balance from blockchain
}

func (w *BitcoinWallet) GenerateAddress() (string, error) {
    // Generate new receiving address
}

func (w *BitcoinWallet) GetUTXOs() ([]UTXO, error) {
    // Get unspent transaction outputs
}
```

### BEEF Transaction Support
```go
type BEEFHandler struct {
    // ... fields
}

func (h *BEEFHandler) CreateBEEFTransaction(inputs []UTXO, outputs []Output) (string, error) {
    // Create BEEF format transaction
}

func (h *BEEFHandler) VerifyBEEFTransaction(beefData string) (bool, error) {
    // Verify BEEF transaction
}

func (h *BEEFHandler) BroadcastBEEFTransaction(beefData string) (string, error) {
    // Broadcast BEEF transaction to miners
}
```

### SPV Verification
```go
type SPVHandler struct {
    // ... fields
}

func (h *SPVHandler) VerifyTransaction(txID string) (bool, error) {
    // Verify transaction using SPV
}

func (h *SPVHandler) GetMerkleProof(txID string) (map[string]interface{}, error) {
    // Get merkle proof for transaction
}

func (h *SPVHandler) VerifyMerkleProof(proof map[string]interface{}) (bool, error) {
    // Verify merkle proof
}
```

## 🔗 Go Daemon HTTP APIs ✅ PRODUCTION READY

### Wallet Management
```http
# Get total balance across all addresses
GET /wallet/balance
Response: {"balance": 29391}

# Generate new HD address
POST /wallet/address/generate
Response: {"address": "1ABC...", "index": 5}

# Get current address
GET /wallet/address/current
Response: {"address": "1ABC...", "index": 5}

# Get all addresses
GET /wallet/addresses
Response: [{"address": "1ABC...", "index": 0, "balance": 1000}, ...]
```

### Transaction Management
```http
# Send complete transaction (create + sign + broadcast)
POST /transaction/send
Content-Type: application/json

{
  "toAddress": "1ABC...",
  "amount": 1000,
  "feeRate": 5
}

Response: {
  "success": true,
  "txid": "bf089bece19a7fac4d7977ba95361075ecc0b87b76a5a68be3ed0e32e0b36286",
  "message": "Transaction sent successfully",
  "whatsOnChainUrl": "https://whatsonchain.com/tx/bf089bece19a7fac4d7977ba95361075ecc0b87b76a5a68be3ed0e32e0b36286"
}
```

### UTXO Management
```http
# Fetch UTXOs for specific address
GET /utxos/{address}
Response: [{"txid": "abc...", "vout": 0, "amount": 1000, "script": "76a9..."}]

# Get UTXO manager status
GET /utxos/status
Response: {"status": "active", "lastUpdate": "2025-09-27T12:43:16Z"}
```

## 🌐 Bitcoin SV Blockchain APIs

### Miner Integration ✅ PRODUCTION READY

#### WhatsOnChain (Primary)
```http
POST https://api.whatsonchain.com/v1/bsv/main/tx/raw
Content-Type: application/json

"hex_encoded_transaction"
```

#### GorillaPool mAPI (Secondary)
```http
POST https://mapi.gorillapool.io/mapi/tx
Content-Type: application/json

{
  "rawtx": "hex_encoded_transaction"
}
```

#### Legacy Miners (Deprecated)
```http
# TAAL Miner (deprecated - endpoint changed)
POST https://api.taal.com/arc/tx

# Terranode Miner (deprecated - endpoint changed)
POST https://api.terranode.io/v1/transactions
```

### Balance & UTXO Queries ✅ PRODUCTION READY

#### Address Balance (WhatsOnChain)
```http
GET https://api.whatsonchain.com/v1/bsv/main/address/{address}/balance
Response: {"balance": 29391, "unconfirmed": 0}
```

#### UTXO List (WhatsOnChain)
```http
GET https://api.whatsonchain.com/v1/bsv/main/address/{address}/unspent
Response: [{"txid": "abc...", "vout": 0, "amount": 1000, "script": "76a9..."}]
```

#### Transaction Details (WhatsOnChain)
```http
GET https://api.whatsonchain.com/v1/bsv/main/tx/{txid}/hex
Response: "0100000001..."
```

#### Legacy APIs (Deprecated)
```http
# TAAL Miner (deprecated)
GET https://api.taal.com/arc/address/{address}/balance
GET https://api.taal.com/arc/address/{address}/utxos
```

## 🔄 Message Flow Architecture ✅ PRODUCTION READY

### Frontend → C++ Bridge → Go Daemon
```typescript
// 1. Frontend calls unified API
const result = await window.bitcoinAPI.sendTransaction({
  toAddress: "1ABC...",
  amount: 1000,
  feeRate: 5
});

// 2. C++ bridge processes message
window.cefMessage.send('send_transaction', [JSON.stringify(data)]);

// 3. Go daemon handles complete transaction flow
POST /transaction/send → Create → Sign → Broadcast → Response
```

## 📊 Current Implementation Status

### ✅ Completed Features
- **HD Wallet System**: BIP44 hierarchical deterministic wallet
- **Transaction Flow**: Complete create + sign + broadcast pipeline
- **Balance Management**: Total balance calculation across all addresses
- **Address Generation**: HD address generation with proper indexing
- **Real Blockchain Integration**: Working with WhatsOnChain and GorillaPool
- **BRC-100 Authentication**: Complete BRC-100 protocol implementation
- **BEEF/SPV Integration**: Real blockchain transactions with SPV verification
- **Frontend Integration**: React UI fully connected to backend
- **Process Isolation**: Each overlay runs in dedicated CEF subprocess

### 🚧 In Development
- **Window Management**: Keyboard commands and overlay HWND movement
- **Transaction Receipt UI**: Improved confirmation and receipt display
- **Frontend BRC-100 Integration**: React authentication modals and approval flows

### 📋 Future Features
- **Transaction History**: Local storage and display
- **Advanced Address Management**: Gap limit, pruning, high-volume generation
- **SPV Verification**: Simplified Payment Verification implementation

## 🔐 BRC-100 Protocol APIs (Future)

### Authentication Flow
```typescript
// 1. App requests identity certificate
const certificate = await window.bitcoinBrowser.identity.getCertificate();

// 2. Wallet provides selective disclosure
const selectiveData = await window.bitcoinBrowser.identity.selectiveDisclosure([
  'publicKey',
  'address'
]);

// 3. App verifies certificate
const isValid = await window.bitcoinBrowser.identity.verifyCertificate(certificate);

// 4. Both parties derive shared keys (Type-42)
const sharedKey = await window.bitcoinBrowser.identity.deriveSharedKey(appPublicKey);

// 5. App creates BEEF transaction
const beefTx = await window.bitcoinBrowser.transactions.createBeef(transactionData);

// 6. Wallet signs BEEF transaction
const signedTx = await window.bitcoinBrowser.transactions.signBeef(beefTx);

// 7. App broadcasts transaction
const txId = await window.bitcoinBrowser.transactions.broadcast(signedTx);
```

## 📊 Data Types

### Identity Data
```typescript
interface IdentityData {
  publicKey: string;
  privateKey: string;  // Only in secure contexts
  address: string;
  backedUp: boolean;
  certificates?: Certificate[];
}
```

### Transaction Data
```typescript
interface TransactionData {
  inputs: UTXO[];
  outputs: Output[];
  fee: number;
  format: 'standard' | 'beef' | 'arc';
}
```

### UTXO
```typescript
interface UTXO {
  txid: string;
  vout: number;
  value: number;
  scriptPubKey: string;
}
```

### Certificate (BRC-52/103)
```typescript
interface Certificate {
  issuer: string;
  subject: string;
  publicKey: string;
  signature: string;
  selectiveDisclosure: string[];
  expiry?: Date;
}
```

## 🚀 Future API Considerations

### Multi-Platform Support
- 🟡 **Windows**: Current CEF implementation
- 🟡 **macOS**: CEF with Cocoa integration
- 🟡 **Mobile**: React Native with native wallet modules

### Advanced Features
- 🟡 **Hardware Security Module (HSM)** integration
- 🟡 **Multi-signature** wallet support
- 🟡 **Smart contract** interaction APIs
- 🟡 **Token gating** and access control

### Performance Optimizations
- 🟡 **Caching** strategies for balance and UTXO data
- 🟡 **Batch operations** for multiple transactions
- 🟡 **WebSocket** connections for real-time updates
- 🟡 **Offline mode** with transaction queuing

---

*This API reference will be updated as the project evolves and new features are implemented.*
