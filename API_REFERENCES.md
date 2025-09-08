# API References - Babbage Browser

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

### Transaction Management
```typescript
// Transaction operations
window.bitcoinBrowser.transactions.create(txData: TransactionData): Promise<Transaction>
window.bitcoinBrowser.transactions.sign(txId: string): Promise<Signature>
window.bitcoinBrowser.transactions.broadcast(tx: Transaction): Promise<BroadcastResult>
window.bitcoinBrowser.transactions.getHistory(): Promise<Transaction[]>
```

### Balance & UTXO Management
```typescript
// Balance and UTXO operations
window.bitcoinBrowser.balance.get(): Promise<number>
window.bitcoinBrowser.utxos.list(): Promise<UTXO[]>
window.bitcoinBrowser.utxos.refresh(): Promise<void>
window.bitcoinBrowser.address.generate(): Promise<string>
```

### Overlay Panel Management
```typescript
// Overlay panel operations
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

## 🔧 CEF ↔ Python Bridge APIs

### Wallet Operations
```cpp
// C++ to Python wallet communication
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

## 🐍 Python Wallet Backend APIs

### Core Wallet Functions
```python
class BitcoinWallet:
    def __init__(self, password: str):
        """Initialize wallet with password"""

    def create_wallet(self) -> dict:
        """Create new wallet and return identity data"""

    def load_wallet(self, private_key: str) -> dict:
        """Load existing wallet from private key"""

    def sign_transaction(self, tx_data: dict) -> str:
        """Sign transaction using bsv-sdk"""

    def get_balance(self) -> int:
        """Get wallet balance from blockchain"""

    def generate_address(self) -> str:
        """Generate new receiving address"""

    def get_utxos(self) -> list:
        """Get unspent transaction outputs"""
```

### BEEF Transaction Support
```python
class BEEFHandler:
    def create_beef_transaction(self, inputs: list, outputs: list) -> str:
        """Create BEEF format transaction"""

    def verify_beef_transaction(self, beef_data: str) -> bool:
        """Verify BEEF transaction"""

    def broadcast_beef_transaction(self, beef_data: str) -> str:
        """Broadcast BEEF transaction to miners"""
```

### SPV Verification
```python
class SPVHandler:
    def verify_transaction(self, tx_id: str) -> bool:
        """Verify transaction using SPV"""

    def get_merkle_proof(self, tx_id: str) -> dict:
        """Get merkle proof for transaction"""

    def verify_merkle_proof(self, proof: dict) -> bool:
        """Verify merkle proof"""
```

## 🌐 Bitcoin SV Blockchain APIs

### Miner Integration

#### TAAL Miner
```http
POST https://api.taal.com/arc/tx
Content-Type: application/json

{
  "rawtx": "hex_encoded_transaction",
  "format": "beef"
}
```

#### GorillaPool Miner
```http
POST https://api.gorillapool.io/api/v1/transactions
Content-Type: application/json

{
  "rawtx": "hex_encoded_transaction"
}
```

#### Terranode Miner
```http
POST https://api.terranode.io/v1/transactions
Content-Type: application/json

{
  "rawtx": "hex_encoded_transaction",
  "format": "arc"
}
```

### Balance & UTXO Queries

#### Address Balance
```http
GET https://api.taal.com/arc/address/{address}/balance
```

#### UTXO List
```http
GET https://api.taal.com/arc/address/{address}/utxos
```

#### Transaction History
```http
GET https://api.taal.com/arc/address/{address}/history
```

### Transaction Status
```http
GET https://api.taal.com/arc/tx/{txid}
```

## 🔐 BRC-100 Protocol APIs

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
