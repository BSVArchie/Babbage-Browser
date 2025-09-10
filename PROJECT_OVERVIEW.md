# Bitcoin-Browser Project Overview

## 🎯 Project Mission

Bitcoin-Browser is a custom Web3 browser built on Chromium Embedded Framework (CEF) that prioritizes security and native control over blockchain operations. Unlike traditional browser-based wallets, this project implements a **Go-based wallet backend** (with future Rust migration possible) for all sensitive wallet operations, ensuring maximum security for real financial transactions.

## 🔒 Security Architecture & Design Philosophy

### Core Security Principle: Native-First Wallet Operations

**The Problem with JavaScript-Based Wallets:**

Traditional browser wallets face significant security challenges because they operate entirely within the JavaScript environment:

1. **Process Isolation Vulnerabilities**
   - JavaScript runs in the browser's render process, which is inherently less secure than native processes
   - The render process is accessible to web content, creating potential attack vectors
   - Browser extensions and injected scripts can potentially intercept wallet operations

2. **Memory Security Issues**
   - Private keys stored in JavaScript variables are accessible through:
     - Browser console inspection
     - Memory dumps and debugging tools
     - Developer tools and extensions
     - Malicious scripts running in the same context
   - Even with encryption, decryption keys must remain in JavaScript

3. **Cross-Site Scripting (XSS) Attack Surface**
   - Malicious websites could potentially access wallet functions through XSS attacks
   - Browser extensions could inject code that accesses wallet data
   - The JavaScript environment is sandboxed but still accessible to web content

**Our Solution: Go Wallet Backend with Process Isolation**

1. **Process Separation**
   - Wallet operations happen in isolated Go daemon processes, completely separate from web content
   - CEF's multi-process architecture provides natural security boundaries
   - Even if a website compromises the render process, it cannot access the wallet backend

2. **Enhanced Memory Protection**
   - Go daemon provides stronger memory protection than JavaScript
   - Can leverage hardware security features and modules (HSM) in future Rust implementation
   - Memory isolation between processes prevents cross-process data access

3. **Cryptographic Library Integration**
   - Direct access to Bitcoin SV Go SDK (bitcoin-sv/go-sdk) with BEEF and SPV support
   - Hardware security module (HSM) integration capabilities planned for production
   - Signing operations happen in isolated, controlled Go daemon environments

4. **Controlled API Exposure**
   - Only safe, high-level functions are exposed through `window.bitcoinBrowser`
   - The bridge API is carefully designed to prevent sensitive data leakage
   - All cryptographic operations remain in the isolated Go backend

### Security Architecture Benefits

- **Real Financial Security**: Built for production use where real money is at stake
- **Regulatory Compliance**: Meets higher security standards required for financial applications
- **Attack Surface Reduction**: Significantly reduces the attack surface compared to JavaScript-based solutions
- **Professional Grade**: Suitable for enterprise and institutional use cases

## 🏗️ Technical Architecture

### Multi-Layer Security Model

```
┌─────────────────────────────────────────────────────────────┐
│                    React UI Layer                          │
│              (TypeScript + Vite)                           │
│              • Wallet UI Components                        │
│              • Settings & Configuration                    │
│              • Navigation Interface                        │
│              🟡 Future: React Native for mobile            │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                JS ↔ Native Bridge Layer                    │
│              • window.bitcoinBrowser API                   │
│              • Controlled Function Exposure                │
│              • No Sensitive Data Transfer                  │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                Native CEF Shell                            │
│              • C++ / Chromium Engine                       │
│              • CEF Event Handlers                          │
│              • Process Isolation                           │
│              🟡 Future: Consider full Chromium build       │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              Go Wallet Backend                             │
│              • bitcoin-sv/go-sdk Integration               │
│              • BEEF Transaction Support                    │
│              • SPV Verification                            │
│              • Secure Key Management                       │
│              🟡 Future: May migrate to Rust for max perf   │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│            Identity & Authentication Layer                 │
│              • BRC-100 Auth Framework                     │
│              • BRC-52/103 Certificates                    │
│              • Type-42 Key Derivation                     │
│              • SPV Identity Validation                     │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              Bitcoin SV Blockchain                        │
│              • TAAL, GorillaPool Miners                   │
│              • Terranode, ARC Formats                     │
│              • Multi-platform Support                     │
│              🟡 Windows, Mac, Mobile builds planned        │
└─────────────────────────────────────────────────────────────┘
```

## 🪟 HWND System & Window Management Architecture

### Window Hierarchy & Creation Flow

The application creates a sophisticated multi-window system with three main HWNDs:

```
┌─────────────────────────────────────────────────────────────┐
│                    Main Shell (g_hwnd)                     │
│              • Parent window for all other HWNDs           │
│              • Handles window resizing and positioning      │
│              • Manages overlay window positioning           │
└─────────────────────┬───────────────────────────────────────┘
                      │
        ┌─────────────┼─────────────┐
        │             │             │
        ▼             ▼             ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│   Header    │ │   WebView   │ │   Overlay   │
│ (g_header_  │ │ (g_webview_ │ │ (g_overlay_│
│   hwnd)     │ │   hwnd)     │ │   hwnd)    │
│             │ │             │ │             │
│ • React UI  │ │ • Web page  │ │ • Dynamic   │
│ • Wallet    │ │ • Content   │ │ • Panels    │
│   buttons   │ │ • Navigation│ │ • Modals    │
│ • Settings  │ │             │ │ • Overlays  │
└─────────────┘ └─────────────┘ └─────────────┘
```

### Window Creation Sequence

1. **Application Startup (WinMain)**
   - Creates main shell window (`g_hwnd`)
   - Creates header window (`g_header_hwnd`) - React UI
   - Creates webview window (`g_webview_hwnd`) - Web content
   - All three are created immediately and assigned to global variables

2. **Overlay Window Creation (On-Demand)**
   - `g_overlay_hwnd` is NOT created at startup
   - Created dynamically when `CreateOverlayBrowserIfNeeded()` is called
   - Triggered by frontend messages requesting overlay panels

### Window Message Processing

**ShellWindowProc (Main Shell)**
- Handles `WM_MOVE` and `WM_SIZE` events
- Automatically repositions overlay window when main window moves/resizes
- Ensures overlay stays properly positioned relative to main window

**OverlayWndProc (Overlay Window)**
- Handles mouse activation and focus management
- Forces overlay to stay on top (`HWND_TOPMOST`)
- Processes mouse clicks and forwards them to CEF browser
- Prevents other windows from covering the overlay

### CEF Browser Integration

Each HWND hosts a separate CEF browser instance:

1. **Header Browser** (`SimpleHandler("header")`)
   - Loads React UI at `http://127.0.0.1:5137`
   - Handles wallet buttons, settings, navigation
   - Runs in the header HWND

2. **WebView Browser** (`SimpleHandler("webview")`)
   - Loads web content (currently `https://www.coingeek.com`)
   - Handles user browsing and navigation
   - Runs in the webview HWND

3. **Overlay Browser** (`SimpleHandler("overlay")`)
   - Loads React overlay UI at `http://127.0.0.1:5137/overlay`
   - Handles panels, modals, and overlay content
   - Runs in the overlay HWND (when created)

### Message Passing & Communication Flow

**Frontend → Backend Communication:**
```
React Component → window.postMessage → CEF Process Message → OnProcessMessageReceived()
```

**Backend → Frontend Communication:**
```
SimpleHandler → ExecuteJavaScript() → React Component State Update
```

**Key Message Types:**
- `"navigate"` - Forward navigation to webview browser
- `"overlay_open_panel"` - Create overlay HWND and show specific panel

### Overlay Panel System Flow

1. **Panel Request**
   - Frontend sends `"overlay_open_panel"` message with panel name
   - Backend receives message in `OnProcessMessageReceived()`

2. **HWND Creation**
   - Calls `CreateOverlayBrowserIfNeeded()` to create overlay window
   - Sets up CEF browser with custom render handler
   - Loads overlay React app

3. **Panel Display**
   - Executes JavaScript to trigger panel: `window.triggerPanel(panelName)`
   - React overlay app renders the requested panel
   - Overlay window becomes visible and interactive

4. **State Management**
   - Frontend and backend both track overlay open/close state
   - Messages can be sent to open and close panels
   - Overlay HWND is created/destroyed as needed

### Render Handler System

**MyOverlayRenderHandler**
- Custom render handler for overlay window
- Handles windowless rendering for transparent overlays
- Manages bitmap creation and layered window updates
- Processes CEF paint events and updates HWND display

**Key Features:**
- Layered window support for transparency
- Custom bitmap management for rendering
- Mouse event forwarding to CEF browser
- DWM composition integration

## 🚀 Development Status

This project is currently in early-stage development with:
- ✅ Basic project structure and architecture defined
- ✅ CEF integration framework established
- ✅ React frontend foundation in place
- 🧱 Native wallet backend implementation in progress
- 🧱 Build system configuration needed
- 🧱 Security testing and validation required

## 🎯 Next Steps

1. **Complete Native Wallet Backend**
   - Implement core cryptographic operations
   - Establish secure key storage mechanisms
   - Create comprehensive security testing suite

2. **Build System Setup**
   - Configure CMake build process
   - Set up development environment
   - Establish CI/CD pipeline

3. **Security Validation**
   - Penetration testing of the native backend
   - Security audit of the bridge API
   - Compliance verification for financial use cases

---

*This document serves as the primary reference for understanding the project's security architecture and development approach.*

## 🎨 Frontend Architecture & React Application Flow

### Application Structure Overview

The frontend is a React application built with TypeScript and Vite, designed to work across multiple CEF browser instances:

```
┌─────────────────────────────────────────────────────────────┐
│                    React Application                       │
│              (Single codebase, multiple instances)        │
└─────────────────────┬───────────────────────────────────────┘
                      │
        ┌─────────────┼─────────────┐
        │             │             │
        ▼             ▼             ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│   Header    │ │   WebView   │ │   Overlay   │
│   Instance  │ │   Instance  │ │   Instance  │
│             │ │             │ │             │
│ • Main.tsx  │ │ • Main.tsx  │ │ • Main.tsx  │
│ • App.tsx   │ │ • App.tsx   │ │ • App.tsx   │
│ • Routes    │ │ • Routes    │ │ • Routes    │
└─────────────┘ └─────────────┘ └─────────────┘
```

### Routing & Instance Detection

**Main.tsx - Entry Point**
- Detects whether running in overlay or header mode via `window.location.pathname`
- Logs the mode for debugging purposes
- Initializes React with BrowserRouter for navigation

**App.tsx - Core Logic & State Management**
- Manages global application state (`walletExists`, `loading`)
- Handles identity loading and wallet existence checks
- Routes between `MainBrowserView` and `OverlayRoot` based on wallet state
- Triggers overlay display when backup modal is needed

### Component Architecture

**Page-Level Components**

1. **MainBrowserView.tsx** - Primary browser interface
   - Navigation toolbar with address bar
   - Wallet and settings buttons
   - Main content area
   - Triggers overlay panels for wallet/settings

2. **OverlayRoot.tsx** - Overlay content manager
   - Manages wallet panel and backup modal states
   - Handles panel triggering from backend messages
   - Controls overlay visibility and cleanup

**Panel Components**

1. **BackupModal.tsx** - Wallet backup interface
   - Displays wallet information (address, public/private keys)
   - Copy-to-clipboard functionality
   - Confirmation checkbox for backup acknowledgment
   - Calls backend `markBackedUp()` function

2. **WalletPanelLayout.tsx** - Wallet management interface
   - Wallet operations and settings
   - Account information display

3. **SettingsPanelLayout.tsx** - Application settings
   - Configuration options
   - User preferences

### State Management & Data Flow

**Identity Management Flow**
```
App.tsx useEffect → Check identity.get() → Set walletExists → Route accordingly
```

**Overlay Panel Flow**
```
Button Click → window.bitcoinBrowser.overlay.show() → Backend creates overlay_hwnd → React renders in overlay
```

**Backup Modal Flow**
```
Identity Check → Show backup modal → User confirms → Call markBackedUp() → Update file → Close modal
```

### API Integration & Hooks

**useBitcoinBrowser Hook**
- `getIdentity()` - Retrieves wallet identity from backend
- `markBackedUp()` - Updates backend backup status
- `navigate()` - Sends navigation commands to webview

**Window Bridge Integration**
- `window.bitcoinBrowser.identity.*` - Backend identity operations
- `window.bitcoinBrowser.overlay.*` - Overlay window management
- `window.bitcoinBrowser.navigation.*` - Browser navigation control
- `window.cefMessage.send()` - Direct CEF message passing

### Type System

**Identity Types**
- `IdentityData` - Complete wallet information with keys and backup status
- `BackupCheck` - Simple backup status confirmation
- `IdentityResult` - Union type for identity operations

**BitcoinBrowser API Types**
- Complete type definitions for all backend API functions
- Overlay management interface types
- Navigation and message passing types

### Current Implementation Status

**✅ Working Components**
- Basic React application structure
- Routing between header and overlay instances
- Wallet panel overlay system
- Backup modal UI and form handling
- Type definitions and API interfaces

**🧱 Partially Implemented**
- Identity file checking and creation
- Backend communication for backup status updates
- Overlay trigger system for backup modal

**❌ Missing/Incomplete**
- Startup identity check integration
- Automatic backup modal display on first launch
- Backend identity file management
- Proper overlay_hwnd creation for backup modal

### Communication Patterns

**Frontend → Backend**
1. **Direct API Calls**: `window.bitcoinBrowser.identity.get()`
2. **Message Passing**: `window.cefMessage.send('overlay_hide', [])`
3. **Overlay Triggers**: `window.bitcoinBrowser.overlay.show()`

**Backend → Frontend**
1. **JavaScript Execution**: `ExecuteJavaScript()` calls
2. **Panel Triggers**: `window.triggerPanel(panelName)`
3. **Message Responses**: Process message callbacks

**State Synchronization**
- Frontend tracks overlay open/close state
- Backend manages HWND creation/destruction
- Both sides coordinate through message passing

## 🔧 Backend Implementation & Core Handlers

### Core Handler Architecture

The backend implements a modular handler system that exposes native functionality to the frontend through V8 JavaScript bindings:

```
┌─────────────────────────────────────────────────────────────┐
│                    V8 JavaScript Context                   │
│              (Frontend React Application)                  │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                Handler Execution Layer                      │
│              • IdentityHandler                             │
│              • PanelHandler                                │
│              • NavigationHandler                           │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                Core Business Logic                         │
│              • WalletManager                               │
│              • File I/O Operations                         │
│              • Cryptographic Functions                     │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                System Integration                          │
│              • Process Message Passing                     │
│              • HWND Management                             │
│              • CEF Browser Control                         │
└─────────────────────────────────────────────────────────────┘
```

### Identity Management System

**WalletManager Class**
- **File Location**: `%APPDATA%/BabbageBrowser/identity.json`
- **Encryption**: AES-256-CBC with hardcoded key (temporary implementation)
- **Key Generation**: ECDSA secp256k1 key pairs
- **Address Format**: Bitcoin-style Base58Check encoding

**Identity File Structure**
```json
{
  "publicKey": "hex_encoded_public_key",
  "address": "base58_encoded_address",
  "privateKey": "aes_encrypted_private_key",
  "backedUp": false
}
```

**Key Operations**
1. **Wallet Creation**: Automatically generates new key pair if none exists
2. **File Persistence**: Saves encrypted identity to AppData directory
3. **Backup Status**: Tracks whether user has acknowledged backup
4. **Key Retrieval**: Decrypts and returns private key when needed

### Handler Implementation Details

**IdentityHandler**
- **Primary Function**: Manages wallet identity operations
- **Methods Exposed**:
  - `get()` - Returns decrypted identity JSON
  - `markBackedUp()` - Updates backup status to true
- **Auto-Creation**: Creates new wallet if none exists

**PanelHandler**
- **Primary Function**: Manages overlay panel display
- **Methods Exposed**:
  - `open(panelName)` - Triggers overlay panel creation
- **Message Flow**: Sends `overlay_open_panel` to browser process

**NavigationHandler**
- **Primary Function**: Handles web navigation requests
- **Methods Exposed**:
  - `navigate(path)` - Forwards navigation to webview browser
- **Message Flow**: Sends `navigate` message to browser process

### Cryptographic Implementation

**Current Implementation (Temporary)**
- **Key Derivation**: ECDSA secp256k1 for Bitcoin compatibility
- **Encryption**: AES-256-CBC with hardcoded key
- **Hashing**: SHA256 + RIPEMD160 for address generation
- **Encoding**: Base58Check for address format

**Security Notes**
- Hardcoded AES key is temporary and will be replaced
- Private keys are encrypted at rest but decrypted in memory
- Key generation uses OpenSSL cryptographic libraries

### File System Integration

**AppData Directory Structure**
```
%APPDATA%/BabbageBrowser/
└── identity.json          # Encrypted wallet identity
```

**File Operations**
- **Creation**: Automatic on first run
- **Reading**: Decryption and JSON parsing
- **Writing**: Encryption and JSON serialization
- **Backup Status**: Persistent boolean flag

### Message Passing Architecture

**Frontend → Backend**
1. **V8 Function Calls**: Direct handler execution
2. **Parameter Validation**: Type checking and error handling
3. **Business Logic**: Core functionality execution

**Backend → Frontend**
1. **Process Messages**: Cross-process communication
2. **JavaScript Execution**: Dynamic code injection
3. **State Updates**: Real-time UI synchronization

### Current Implementation Status

**✅ Fully Implemented**
- Wallet key generation and management
- Identity file creation and persistence
- AES encryption/decryption of private keys
- Base58 address generation
- Basic handler execution framework
- Process message passing system

**🧱 Partially Implemented**
- Backup modal integration with overlay system
- Startup identity check automation
- Error handling and validation

**❌ Missing/Incomplete**
- Secure key storage (replacing hardcoded AES key)
- Hardware security module integration
- Comprehensive error handling
- Backup modal automatic display on first launch

### Technical Debt & Future Improvements

**Immediate Concerns**
- Hardcoded AES encryption key (security risk)
- Limited error handling and validation
- No hardware security integration

**Planned Refactoring**
- Replace C++ implementation with alternative language
- Implement proper key derivation and storage
- Add comprehensive security auditing
- Integrate with hardware security modules

**Current Priority**
- Get backup modal flow working with existing implementation
- Establish proper overlay integration
- Complete the user experience flow
- Address security improvements in subsequent iterations
