+----------------------------+
|        React UI Layer     |
|  - Panels / Pages / Hooks |
|  - TypeScript + Vite      |
|  - Transaction Forms      |
|  - Balance Display        |
|  - Address Management     |
+----------------------------+
            ↓
+----------------------------+
|   JS ↔ Native Bridge Layer |
|  - window.bitcoinBrowser   |
|  - window.cefMessage       |
|  - Process Communication   |
+----------------------------+
            ↓
+----------------------------+
|     Native CEF Shell       |
|  - C++ / Chromium          |
|  - CEF Handlers            |
|  - Process-Per-Overlay     |
|  - Message Routing         |
+----------------------------+
            ↓
+----------------------------+
|   Go Wallet Backend        |
|  - bitcoin-sv/go-sdk       |
|  - HD Wallet (BIP44)       |
|  - Transaction Creation    |
|  - Transaction Signing     |
|  - Transaction Broadcasting|
|  - UTXO Management         |
|  - Real Blockchain APIs    |
+----------------------------+
            ↓
+----------------------------+
| Bitcoin SV Blockchain      |
|  - WhatsOnChain API        |
|  - GorillaPool mAPI        |
|  - Real Transaction IDs    |
|  - On-chain Verification   |
+----------------------------+
            ↓
+----------------------------+
| Identity & Auth Layer      | 🚧 FUTURE
|  - BRC-100 Auth Framework  |
|  - BRC-52/103 Certificates |
|  - Type-42 Key Derivation  |
|  - Selective Disclosure    |
|  - SPV Identity Validation |
+----------------------------+



Process-Per-Overlay Communication Architecture
┌─────────────────┐    V8 Injection    ┌─────────────────┐
│   C++ Backend   │ ──────────────────→ │  Render Process │
│                 │                     │                 │
│ • Wallet APIs   │                     │ • window.bitcoinAPI.sendTransaction() │
│ • Address Mgmt  │                     │ • window.bitcoinAPI.getBalance() │
│ • Overlay Mgmt  │                     │ • window.cefMessage.send() │
│ • Message Handlers│                   │ • window.bitcoinBrowser.address.* │
└─────────────────┘                     └─────────────────┘
         │                                        │
         │ Process Messages                       │ JavaScript Execution
         │                                        │
         ▼                                        ▼
┌─────────────────┐                     ┌─────────────────┐
│ Browser Process │                     │   React App     │
│ Message Handler │                     │                 │
│                 │                     │ • Transaction UI │
│ • send_transaction│                   │ • Balance Display│
│ • get_balance   │                     │ • Address Gen   │
│ • overlay_show_*│                     │ • Process isolation │
│ • overlay_close │                     │ • Real-time Updates│
└─────────────────┘                     └─────────────────┘

## 🔄 Transaction Flow Architecture

### Complete Transaction Pipeline
```
React UI → C++ Bridge → Go Daemon → Blockchain
    ↓           ↓           ↓           ↓
1. User Input  2. Message   3. Create    4. Broadcast
   (Amount)    Routing      Transaction  to Miners
    ↓           ↓           ↓           ↓
5. Confirmation 6. Response 7. Sign     8. Real TxID
   Modal        Handling    Transaction  Returned
    ↓           ↓           ↓           ↓
9. Success     10. UI      11. UTXO    12. On-chain
   Display      Update      Selection   Verification
```

### Process Isolation Architecture
┌─────────────────────────────────────────────────────────────┐
│                    Main Browser Process                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐  │
│  │   Header HWND   │  │  WebView HWND   │  │ Main Shell  │  │
│  │ React App (/)   │  │ Web Content     │  │ Management  │  │
│  └─────────────────┘  └─────────────────┘  └─────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Process-Per-Overlay Architecture               │
│                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐  │
│  │ Settings Overlay│  │  Wallet Overlay │  │Backup Modal │  │
│  │   Process       │  │    Process      │  │   Process   │  │
│  │                 │  │                 │  │             │  │
│  │ ┌─────────────┐ │  │ ┌─────────────┐ │  │┌───────────┐│  │
│  │ │Settings HWND│ │  │ │ Wallet HWND │ │  ││Backup HWND││  │
│  │ │/settings    │ │  │ │ /wallet     │ │  ││/backup    ││  │
│  │ │Fresh V8     │ │  │ │Fresh V8     │ │  ││Fresh V8   ││  │
│  │ │Context      │ │  │ │Context      │ │  ││Context    ││  │
│  │ └─────────────┘ │  │ └─────────────┘ │  │└───────────┘│  │
│  └─────────────────┘  └─────────────────┘  └─────────────┘  │
└─────────────────────────────────────────────────────────────┘



flowchart TD
    A["Babbage-Browser (BitcoinBrowser)"]

    A --> B["Backend (C++ Shell)"]
    A --> C["Frontend (React + Vite)"]
    A --> D["Build System (CMake)"]
    A --> E["External Dependencies"]
    A --> F["Documentation"]
    A --> G["Project Configuration"]

    B --> B1["CEF Browser Engine"]
    B --> B2["Core Business Logic"]
    B --> B3["CEF Event Hooks"]
    B --> B4["Overlay System"]
    B --> B5["V8 Integration"]

    B1 --> B1a["Main Shell (cef_browser_shell.cpp)"]
    B1 --> B1b["Browser Process Handler (SimpleApp)"]
    B1 --> B1c["Render Process Handler (SimpleRenderProcessHandler)"]
    B1 --> B1d["Client Handler (SimpleHandler)"]

    B2 --> B2a["Wallet Manager (WalletManager.cpp)"]
    B2 --> B2b["Identity Handler (IdentityHandler.cpp)"]
    B2 --> B2c["Navigation Handler (NavigationHandler.cpp)"]
    B2 --> B2d["Panel Handler (PanelHandler.cpp)"]

    B2a --> B2a1["Identity Management"]
    B2a --> B2a2["Key Generation (EC, OpenSSL)"]
    B2a --> B2a3["AES Encryption/Decryption"]
    B2a --> B2a4["Base58 Encoding"]
    B2a --> B2a5["File I/O (wallet.json)"]

    B2b --> B2b1["get() - Retrieve wallet identity"]
    B2b --> B2b2["markBackedUp() - Mark wallet as backed up"]

    B2c --> B2c1["navigate() - Handle URL navigation"]

    B2d --> B2d1["open() - Open overlay panels"]

    B3 --> B3a["Life Span Handler (OnAfterCreated, OnLoadingStateChange)"]
    B3 --> B3b["Display Handler (OnTitleChange)"]
    B3 --> B3c["Load Handler (OnLoadError, OnLoadingStateChange)"]
    B3 --> B3d["Process Message Handler (OnProcessMessageReceived)"]

    B4 --> B4a["Process-Per-Overlay System"]
    B4 --> B4b["Dedicated HWND Management"]
    B4 --> B4c["Custom Render Handler (MyOverlayRenderHandler)"]
    B4 --> B4d["Window Message Handlers (WndProc)"]

    B5 --> B5a["Context Creation (OnContextCreated)"]
    B5 --> B5b["Native API Injection (window.bitcoinBrowser)"]
    B5 --> B5c["Function Binding (CefV8Value::CreateFunction)"]

    C --> C1["Application Structure"]
    C --> C2["Pages"]
    C --> C3["Components"]
    C --> C4["Hooks"]
    C --> C5["Types"]
    C --> C6["Bridge Layer"]
    C --> C7["Development Server"]

    C1 --> C1a["Main Entry (main.tsx)"]
    C1 --> C1b["App Component (App.tsx)"]
    C1 --> C1c["Router Configuration (BrowserRouter)"]

    C2 --> C2a["Welcome (/welcome) - Initial wallet setup"]
    C2 --> C2b["Main Browser View (/) - Primary dashboard"]
    C2 --> C2c["Settings Overlay (/settings) - Settings panel"]
    C2 --> C2d["Wallet Overlay (/wallet) - Wallet panel"]
    C2 --> C2e["Backup Modal (/backup) - Identity backup"]

    C3 --> C3a["Panels"]
    C3 --> C3b["Main Browser Interface"]

    C3a --> C3a1["Wallet Panel Layout (WalletPanelLayout.tsx)"]
    C3a --> C3a2["Wallet Panel Content (WalletPanelContent.tsx)"]
    C3a --> C3a3["Settings Panel Layout (SettingsPanelLayout.tsx)"]
    C3a --> C3a4["Backup Modal (BackupModal.tsx)"]

### ✅ Completed Components
- **React UI Layer**: Complete with transaction forms, balance display, address management
- **C++ Bridge Layer**: Full message handling and API injection
- **Go Wallet Daemon**: Complete HD wallet with transaction processing
- **Process Isolation**: Each overlay runs in dedicated CEF subprocess
- **Blockchain Integration**: Working with real Bitcoin SV network

### 🚧 In Development
- **Window Management**: Keyboard commands and overlay HWND movement
- **Transaction Receipt UI**: Improved confirmation and receipt display
- **BRC-100 Authentication**: Identity management system integration

### 📋 Future Components
- **Transaction History**: Local storage and display
- **Advanced Address Management**: Gap limit, pruning, high-volume generation
- **SPV Verification**: Simplified Payment Verification implementation
