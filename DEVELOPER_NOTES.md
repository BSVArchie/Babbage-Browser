# Developer Notes - Bitcoin Browser

## 🚨 **CURRENT SESSION STATUS - READ THIS FIRST**

### **Current State: Bitcoin SV Wallet System - FULL TRANSACTION FLOW COMPLETE ✅**

**✅ WHAT'S WORKING:**
1. **HD Wallet System**: BIP44 hierarchical deterministic wallet implementation complete
2. **Unified Wallet Storage**: Single `wallet.json` file with mnemonic, addresses, and metadata
3. **Address Generation**: Working HD address generation with proper key derivation
4. **Transaction Creation**: Complete transaction building with UTXO selection and fee calculation
5. **Transaction Signing**: ✅ **COMPLETE** - Full transaction signing using BSV SDK with source transactions
6. **Transaction Broadcasting**: ✅ **COMPLETE** - Successfully broadcasting to multiple BSV miners
7. **UTXO Management**: Real-time UTXO fetching from multiple BSV APIs
8. **Process-Per-Overlay Architecture**: Each overlay runs in its own dedicated CEF subprocess
9. **Settings Overlay**: Opens in separate process with proper window management
10. **Wallet Overlay**: Runs in isolated process with fresh V8 context
11. **API Injection**: `bitcoinBrowser` API properly injected into each overlay process
12. **Message Handling**: All IPC messages working correctly between processes
13. **USD Price Display**: Real-time BSV to USD conversion using CryptoCompare API
14. **Unified Wallet UI**: Clean interface with Send/Receive buttons and transaction forms
15. **End-to-End Testing**: ✅ **COMPLETE** - Complete transaction flow validated with on-chain transactions
16. **Frontend Integration**: ✅ **NEW** - React UI fully integrated with backend via C++ bridge
17. **Real Transaction IDs**: ✅ **NEW** - Frontend displays actual blockchain transaction IDs
18. **Balance Display**: ✅ **NEW** - Total balance calculation across all addresses working
19. **Transaction Confirmation**: ✅ **NEW** - Complete transaction flow with confirmation modals

**❌ WHAT'S NOT WORKING:**
1. **Transaction History**: Not yet implemented
2. **Advanced Address Management**: Gap limit, pruning, and high-volume address generation
3. **Window Management Issues**: Keyboard commands, overlay HWND movement with main window
4. **Transaction Receipt Display**: Needs improved UI for transaction confirmation

**❌ CRITICAL ISSUE - BACKUP MODAL RENDERING:**
- **Problem**: Backup modal overlay opens but JSX content doesn't display in HWND
- **Status**: Intermittent - works on first run, fails on subsequent runs
- **Root Cause**: CEF overlay state corruption between runs
- **Impact**: Blocks wallet initialization for new users
- **Workaround**: Modal will be removed in production (moved to installation process)

**✅ ARCHITECTURE ACHIEVEMENTS:**
- **Complete Process Isolation**: No more state pollution between overlays
- **Dedicated HWNDs**: Each overlay type has its own window class and message handler
- **Fresh V8 Context**: Each overlay gets clean JavaScript context
- **Proper Cleanup**: Windows can be opened and closed multiple times
- **DevTools Integration**: Right-click "Inspect Element" works for all overlays
- **Real Bitcoin SV Integration**: Working UTXO fetching from multiple BSV APIs
- **Transaction Building**: Complete transaction creation with real UTXO selection and fee calculation
- **Multi-Miner Broadcasting**: Support for TAAL, GorillaPool, Teranode, and WhatsOnChain
- **Address Generation**: Working address generation with clipboard integration
- **USD Price Integration**: Real-time price conversion using CryptoCompare API
- **Unified Wallet Interface**: Clean, modern wallet UI with Send/Receive functionality

### **🎉 MAJOR ACHIEVEMENTS - FRONTEND INTEGRATION COMPLETE:**

**✅ COMPLETED IN THIS SESSION (2025-09-27):**
1. **Fixed Transaction Flow**: Resolved complete transaction creation, signing, and broadcasting pipeline
2. **Fixed Amount Conversion**: Corrected BSV to satoshis conversion (parseInt → parseFloat with multiplication)
3. **Fixed Transaction ID Extraction**: Implemented proper extraction of real transaction IDs from GorillaPool responses
4. **Fixed Balance Display**: Connected frontend to backend for total balance calculation across all addresses
5. **Added Missing Message Handlers**: Implemented C++ renderer process handlers for transaction responses
6. **Unified Transaction API**: Created single `/transaction/send` endpoint combining create+sign+broadcast
7. **Frontend Integration**: Connected React UI to native C++ bridge for complete wallet operations
8. **Real Blockchain Integration**: Transactions now visible on WhatsOnChain with correct transaction IDs

**🔧 TECHNICAL FIXES IMPLEMENTED:**
- Fixed hardcoded transaction IDs in Go daemon
- Fixed BSV amount parsing in frontend (0.00000546 BSV → 546 satoshis)
- Fixed GorillaPool response parsing to extract real transaction IDs
- Fixed missing C++ message handlers for frontend communication
- Fixed balance calculation to use live UTXO data from all addresses
- Fixed transaction ID extraction from miner responses

### **🎉 MAJOR ACHIEVEMENTS - TRANSACTION SYSTEM COMPLETE:**

**✅ COMPLETE TRANSACTION FLOW IMPLEMENTED:**
- **Problem**: Transaction signing and broadcasting were failing
- **Root Cause**: BSV SDK `tx.Sign()` requires source transactions and unlocking script templates
- **Solution**: Store transaction object with source transactions instead of parsing from hex
- **Result**: ✅ **SUCCESS** - Transactions now sign and broadcast successfully to BSV network

**✅ TRANSACTION SIGNING BREAKTHROUGH:**
- **Issue**: `tx.Sign()` was not working because inputs lacked source transaction data
- **Discovery**: Parsing transaction from hex loses `SourceTransaction` and `UnlockingScriptTemplate`
- **Fix**: Store the original transaction object in `WalletService.createdTransaction`
- **Outcome**: Transactions now sign correctly with proper unlocking scripts

**✅ BROADCASTING SUCCESS:**
- **Miners**: Successfully broadcasting to WhatsOnChain and GorillaPool
- **Validation**: Transactions confirmed on-chain with valid transaction IDs
- **Example**: `38ac3f46a5e1d5c79dc0e697f15f6bf1cc4cb0114877b8e01c2690a8aac361e5`

**✅ UNIFIED WALLET SYSTEM:**
- **Problem**: Separate identity and wallet files causing complexity
- **Solution**: Single `wallet.json` file with mnemonic, addresses, and metadata
- **Result**: Simplified wallet management with proper backup functionality

**✅ HD WALLET IMPLEMENTATION:**
- **Problem**: Single private key system with no address persistence
- **Solution**: Complete BIP44 HD wallet with mnemonic backup and address derivation
- **Result**: Professional wallet with proper key management and address tracking

**✅ ADDRESS GENERATION SYSTEM FIXED:**
- **Problem**: Address generation was returning `undefined` due to V8 function override conflicts
- **Solution**: Forced override of `window.bitcoinBrowser.address.generate()` to use message-based system
- **Result**: Address generation now works perfectly with auto-copy to clipboard and user feedback

**✅ USD PRICE DISPLAY IMPLEMENTED:**
- **Problem**: CoinGecko API was blocked by CORS restrictions
- **Solution**: Implemented multiple price sources with CryptoCompare API as primary
- **Result**: Real-time BSV to USD conversion working ($24.58 current price)

**✅ WALLET UI ENHANCED:**
- **Added**: Receive button with address generation and clipboard copy
- **Added**: User feedback message when address is copied
- **Added**: USD price display alongside BSV balance
- **Result**: Complete wallet interface ready for real transactions

**✅ SYSTEM INTEGRATION COMPLETED:**
- **Frontend → C++ → Go Daemon**: Complete message flow working
- **Address Generation**: Go daemon → C++ handler → Frontend display
- **Balance Display**: Real UTXO fetching and USD conversion
- **User Experience**: Smooth, responsive interface with proper feedback

**❌ BACKUP MODAL RENDERING ISSUE - EXTENSIVE DEBUGGING SESSION:**
- **Problem**: Backup modal overlay opens but JSX content doesn't display in HWND
- **Status**: Intermittent - works on first run, fails on subsequent runs
- **Time Spent**: Hundreds of hours debugging this specific issue
- **Root Cause**: CEF overlay state corruption between runs, not React rendering issues
- **Attempted Solutions**:
  - C++ Backend State Management (5 phases implemented)
  - localStorage persistence fixes
  - Ready State Pattern implementation
  - Visual debugging with bright colors
  - Complete overlay recreation instead of reuse
  - Memory management improvements
  - Timing synchronization fixes
- **Research**: Extensive web research on CEF + React state reset issues
- **Conclusion**: This is a deep CEF internals issue with overlay HWND rendering
- **Workaround**: Modal will be removed in production (moved to installation process)

### **🔧 CURRENT TRANSACTION SYSTEM STATUS:**

**✅ WHAT'S WORKING:**
- **HD Wallet System**: BIP44 hierarchical deterministic wallet implementation complete
- **Transaction Creation**: Complete transaction building with UTXO selection and fee calculation
- **UTXO Fetching**: Real-time UTXO fetching from multiple BSV APIs
- **Transaction Serialization**: Proper Bitcoin transaction format using BSV SDK
- **Multi-Miner Broadcasting**: Support for TAAL, GorillaPool, Teranode, and WhatsOnChain
- **Address Generation**: Working HD address generation with proper key derivation
- **Unified Wallet Storage**: Single `wallet.json` file with mnemonic, addresses, and metadata
- **Balance Display**: Shows total balance across all addresses

**❌ WHAT'S NOT WORKING:**
- **Transaction Signing**: Not yet tested with real transactions
- **Transaction Broadcasting**: Not yet tested with real transactions
- **Frontend Integration**: Not yet integrated with React UI
- **Backup Modal Rendering**: CEF overlay state corruption issue (non-blocking)

**🔍 CURRENT TESTING STATUS:**
- **Transaction Creation**: ✅ Working - creates transactions with real UTXO data
- **Transaction Signing**: ✅ **NEW** - Working with BSV SDK and source transactions
- **Transaction Broadcasting**: ✅ **NEW** - Successfully broadcasting to multiple miners
- **UTXO Management**: ✅ Working - fetches and manages UTXOs from multiple addresses
- **Address Generation**: ✅ Working - generates HD addresses with proper derivation
- **Balance Display**: ✅ Working - shows total balance across all addresses
- **Frontend Integration**: ❌ Not tested - needs React UI integration
- **End-to-End Testing**: ✅ **NEW** - Complete transaction flow validated with on-chain transactions

**📋 HD WALLET IMPLEMENTATION COMPLETE:**
- `go-wallet/main.go` - ✅ Updated with HD wallet system
- `go-wallet/hd_wallet.go` - ✅ Complete HD wallet implementation
- `frontend/src/hooks/useBalance.ts` - ✅ Updated to use HD wallet addresses
- `frontend/src/hooks/useTransaction.ts` - ✅ Updated to use HD wallet addresses
- `frontend/src/components/panels/WalletPanelContent.tsx` - ✅ Updated address generation
- `frontend/src/components/panels/BackupModal.tsx` - ✅ Updated to show mnemonic instead of private key

### **🚀 NEXT DEVELOPMENT PRIORITIES:**

**IMMEDIATE NEXT STEPS (High Priority):**
1. **FRONTEND TRANSACTION INTEGRATION**: Connect React UI to Go daemon transaction endpoints
   - **Current**: Backend transaction flow complete and tested
   - **Goal**: Integrate transaction creation, signing, and broadcasting with React UI
   - **Impact**: Enable users to send transactions through the wallet interface

2. **TRANSACTION HISTORY IMPLEMENTATION**: Display recent transactions in wallet panel
   - **Current**: Backend can create and broadcast transactions
   - **Goal**: Show transaction history with status, amounts, and transaction IDs
   - **Impact**: Users can see their transaction history and status

3. **REAL-TIME BALANCE UPDATES**: Update balances after successful transactions
   - **Current**: Balances show correctly but don't update after transactions
   - **Goal**: Automatically refresh balances after transaction completion
   - **Impact**: Users see updated balances immediately after sending transactions

4. **ERROR HANDLING & USER FEEDBACK**: Comprehensive error handling for transaction failures
   - **Current**: Backend has error handling, frontend needs user feedback
   - **Goal**: Show clear error messages and success confirmations
   - **Impact**: Better user experience with clear feedback

5. **SKIP BACKUP MODAL ISSUE**: Move backup modal to installation process
   - **Current**: Modal blocks wallet initialization due to rendering issues
   - **Goal**: Remove modal from startup, add to installation/setup process
   - **Impact**: Unblocks core wallet development

**🎯 FRONTEND INTEGRATION TASKS:**

1. **TransactionForm.tsx Integration**:
   - Connect form submission to `/transaction/create` endpoint
   - Handle transaction creation response
   - Call `/transaction/sign` endpoint with created transaction
   - Call `/transaction/broadcast` endpoint with signed transaction
   - Show loading states during each step
   - Display success/error messages

2. **useTransaction.ts Hook**:
   - Implement transaction creation function
   - Implement transaction signing function
   - Implement transaction broadcasting function
   - Handle error states and loading states
   - Return transaction status and results

3. **useBalance.ts Updates**:
   - Add function to refresh balance after transaction
   - Update balance display when transaction completes
   - Handle balance updates in real-time

4. **WalletPanelContent.tsx Updates**:
   - Add transaction history display
   - Show recent transactions with status
   - Add transaction details modal
   - Update balance display after transactions

### **🔧 TRANSACTION SYSTEM IMPLEMENTATION COMPLETE:**

**✅ Phase 1: Real Transaction Serialization and Signing - COMPLETE**
- **File**: `go-wallet/transaction_builder.go` - ✅ Complete transaction building with BSV SDK
- **File**: `go-wallet/transaction_broadcaster.go` - ✅ Multi-miner broadcasting support
- **File**: `go-wallet/utxo_manager.go` - ✅ UTXO fetching and management
- **File**: `go-wallet/main.go` - ✅ HTTP API endpoints for transaction operations

**✅ Key Technical Achievements:**
- **BSV SDK Integration**: Proper use of `go-sdk` for transaction building and signing
- **Source Transaction Handling**: Fetching and storing source transactions for signing
- **Unlocking Script Templates**: Using `p2pkh.Unlock()` for proper script templates
- **Transaction Object Persistence**: Storing transaction objects instead of parsing from hex
- **Multi-Miner Broadcasting**: Support for WhatsOnChain and GorillaPool
- **Rate Limiting Handling**: Retry logic for API rate limits
- **Error Handling**: Comprehensive error handling and logging

**✅ HD WALLET IMPLEMENTATION COMPLETE:**

**✅ Phase 1: Core HD Wallet (Go Daemon) - COMPLETE**
- **File**: `go-wallet/hd_wallet.go` - ✅ Complete HD wallet implementation
- **Storage**: `C:\Users\archb\AppData\Roaming\BabbageBrowser\wallet\wallet.json`
- **Features**: ✅ Mnemonic generation, address derivation, address tracking
- **BIP44 Path**: `m/44'/236'/0'/0/{index}` (BSV uses coin type 236)

**✅ Phase 2: Frontend Integration - COMPLETE**
- **Update**: ✅ All frontend hooks updated to use HD wallet endpoints
- **Remove**: ✅ Identity-based address generation removed
- **Add**: ✅ Mnemonic backup UI in existing backup modal
- **Update**: ✅ Balance display for multiple addresses

**✅ Phase 3: Testing & Polish - COMPLETE**
- **Test**: ✅ Address generation and transaction flow working
- **Test**: ✅ Mnemonic backup and restore working
- **Test**: ✅ Balance aggregation across addresses working
- **Polish**: ✅ UI/UX improvements implemented

**✅ Key Design Decisions Implemented:**
- **Migration**: ✅ Immediate replacement of identity system
- **Mnemonic Backup**: ✅ Show in existing backup modal with copy-to-clipboard
- **Address Generation**: ✅ On-demand (can change to batch later)
- **Balance Checking**: ✅ Check all addresses on every request (no caching for now)

**MEDIUM PRIORITY:**
4. **Currency Toggle**: BSV/USD input switching
5. **Production Build**: Disable DevTools and context menu for security
6. **Transaction Feedback**: Enhanced success/error messaging

**LOW PRIORITY:**
7. **Paymail Integration**: Simplified address input
8. **Auto-Pay System**: Micro-transaction approvals
9. **Exchange Integration**: Built-in trading functionality

### **Architecture Overview:**
- **Main Browser**: React app running on `http://127.0.0.1:5137` (header browser)
- **Settings Overlay**: Separate CEF process on `http://127.0.0.1:5137/settings`
- **Wallet Overlay**: Separate CEF process on `http://127.0.0.1:5137/wallet`
- **Backup Modal Overlay**: Separate CEF process on `http://127.0.0.1:5137/backup`
- **Communication**: Uses `cefMessage.send()` for IPC between processes
- **Window Management**: Each overlay has dedicated HWND with custom WndProc handlers

### **Key Architectural Changes:**
1. **Removed Shared Overlay System**: Eliminated `g_overlay_hwnd` and shared overlay reuse
2. **Process-Per-Overlay**: Each overlay type creates its own CEF subprocess
3. **Dedicated Window Classes**: `CEFSettingsOverlayWindow`, `CEFWalletOverlayWindow`, `CEFBackupOverlayWindow`
4. **Custom Message Handlers**: Each overlay has its own `WndProc` for mouse input and window management
5. **Identity System Rebuild**: Complete rewrite with proper file management and Go daemon integration

### **Technical Context:**
- **CEF Version**: Using CEF binaries with process-per-overlay architecture
- **React**: Frontend running on Vite dev server with React Strict Mode disabled
- **Build System**: CMake with Visual Studio
- **Debug Logging**: All native logs go to `debug_output.log`
- **Go Daemon**: Wallet backend with automatic startup and HTTP API integration
- **Bitcoin SV SDK**: Using `bitcoin-sv/go-sdk` for cryptographic operations
- **UTXO APIs**: WhatsOnChain and Bitails for real-time UTXO data
- **Miner APIs**: TAAL, GorillaPool, Teranode, and WhatsOnChain for broadcasting

---

## 🚀 **NEW: Bitcoin SV Transaction System Implementation**

### **✅ COMPLETED: Complete Transaction Backend (Current Session)**

**Backend Architecture:**
```
React UI → C++ Bridge → Go Daemon → Bitcoin SV Network
    ↓           ↓           ↓            ↓
Transaction  Message    Transaction   Blockchain
   Form      Handlers     Logic       Broadcasting
```

**Go Daemon Components:**
1. **`main.go`** - HTTP server with transaction endpoints
2. **`utxo_manager.go`** - Real UTXO fetching from BSV APIs
3. **`transaction_builder.go`** - Transaction creation and signing
4. **`transaction_broadcaster.go`** - Multi-miner broadcasting

**HTTP Endpoints Implemented:**
- `GET /health` - Health check
- `GET /identity/get` - Get wallet identity
- `POST /identity/markBackedUp` - Mark wallet as backed up
- `GET /address/generate` - Generate new Bitcoin address
- `GET /utxo/fetch?address=ADDRESS` - Fetch UTXOs for address
- `POST /transaction/create` - Create unsigned transaction
- `POST /transaction/sign` - Sign transaction
- `POST /transaction/broadcast` - Broadcast transaction to BSV network

**UTXO Management:**
- **Primary API**: WhatsOnChain (most reliable for BSV)
- **Fallback API**: Bitails
- **Error Handling**: Graceful fallback if APIs fail
- **Real Data**: Successfully tested with Genesis address (1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa)

**Transaction Building:**
- **UTXO Selection**: Largest-first coin selection algorithm
- **Fee Calculation**: Dynamic fee calculation based on transaction size
- **Change Output**: Automatic change calculation and output creation
- **Sender Address**: Supports both wallet identity and custom sender addresses

**Multi-Miner Broadcasting:**
- **TAAL**: Major BSV mining pool
- **GorillaPool**: Another major BSV mining pool
- **Teranode**: BSV node implementation
- **WhatsOnChain**: BSV relay network
- **Fallback Strategy**: Try all miners, succeed if any accept

**Security Features:**
- **Private Key Protection**: Private keys never exposed in API responses
- **Process Isolation**: Transaction logic runs in isolated Go daemon
- **Error Handling**: Secure error messages that don't leak sensitive data

**Testing Results:**
- ✅ **UTXO Fetching**: Successfully fetches real UTXOs from BSV network
- ✅ **Transaction Creation**: Successfully creates transactions with real UTXO data
- ✅ **Fee Calculation**: Accurate fee calculation (tested: 380 satoshis for sample transaction)
- ✅ **Error Handling**: Proper handling of addresses with no UTXOs
- ✅ **API Integration**: All endpoints responding correctly

**Current Limitations:**
- **Transaction Serialization**: Using placeholder serialization (needs real Bitcoin format)
- **Transaction Signing**: Using placeholder signing (needs real cryptographic implementation)
- **HD Wallets**: Single address per wallet (needs BIP32/BIP44 implementation)

---

## 🎯 Current Development Focus: Bitcoin SV Transaction System - Frontend Integration

### 🚀 NEW FOCUS: Complete Transaction System Implementation
- **Status**: Backend complete, frontend integration needed
- **Goal**: Complete end-to-end Bitcoin SV transaction system
- **Current Achievement**: Working Go daemon with real UTXO fetching and transaction building
- **Next Target**: React frontend components and C++ bridge integration

## 📊 Comprehensive Architecture Analysis

### Current Overlay System (Single Process Model)
- **Architecture**: All overlays run in same CEF process
- **HWND Management**: Reuses `g_overlay_hwnd` via `CreateOverlayBrowserIfNeeded()`
- **State Management**: Shared V8 context causes state pollution
- **Communication**: Uses `window.cefMessage.send()` for IPC
- **Problems**:
  - Settings button opens in header HWND instead of overlay
  - Modal JSX doesn't populate due to state pollution
  - No process isolation between overlays

### Target Architecture (Process-Per-Overlay Model)
- **Architecture**: Each overlay runs in separate CEF subprocess
- **HWND Management**: Creates new HWND per overlay type
- **State Management**: Complete V8 context isolation
- **Communication**: Process-based IPC with fresh state
- **Benefits**:
  - Clean state for each overlay
  - Better security through process boundaries
  - No state pollution between overlays
  - Mimics Brave Browser architecture

## 🎯 Current Development Focus: Complete C++ to Go Integration ✅

### ✅ COMPLETED: Full C++ to Go Daemon Integration
- **Status**: Complete end-to-end integration working perfectly
- **Architecture**: React → C++ WalletService → Go Daemon → Response
- **Process Management**: Automatic daemon startup, monitoring, and cleanup
- **HTTP Communication**: Windows HTTP API client for all wallet operations
- **Testing**: Full pipeline tested and working seamlessly

### ✅ COMPLETED: Go Wallet Backend Implementation
- **Status**: Fully functional Go wallet daemon using bitcoin-sv/go-sdk
- **Features**: Identity creation, loading, saving, HTTP API endpoints
- **Testing**: All endpoints tested and working (health, identity/get, identity/markBackedUp)
- **Documentation**: Updated all docs with Go dependencies and build instructions

### Architecture Decision: Go for PoC, Rust Possible for Future
- **Current**: Go wallet backend using bitcoin-sv/go-sdk for rapid development
- **Future**: May migrate to Rust for maximum performance and memory safety
- **Rationale**: Go provides excellent balance of performance, safety, and development speed with official BSV SDK support

### Key Considerations for Future Development
- 🟡 **CEF vs Full Chromium**: Consider building full Chromium for better control
- 🟡 **React vs React Native**: Evaluate React Native for mobile compatibility
- 🟡 **Multi-platform**: Plan for Windows, Mac, and mobile builds
- 🟡 **Key Derivation**: PBKDF2-SHA256 is Bitcoin standard (Argon2 optional for production)
- 🟡 **Go to Rust**: Consider migration path for maximum performance if needed

## 🚀 Process-Per-Overlay Implementation Plan

### Phase 1: Analysis and Preparation ✅
- **Current State Assessment**: ✅ Completed
  - Wallet button works (reuses existing overlay)
  - Settings button broken (opens in header instead of overlay)
  - Modal JSX not populating due to state pollution
  - No process isolation
- **Architecture Research**: ✅ Completed
  - Analyzed current single-process model
  - Researched Brave Browser process-per-overlay architecture
  - Identified key differences and benefits

### Phase 2: Settings Button Process-Per-Overlay Implementation
**Goal**: Fix settings button to open dedicated overlay with fresh process

#### Step 2.1: Modify Settings Button Handler
**File**: `cef-native/src/handlers/simple_handler.cpp`
**Location**: `OnProcessMessageReceived()` method
```cpp
if (message_name == "overlay_show") {
    // Check if this is settings overlay request
    if (role_ == "header") {
        // Create new process for settings overlay
        CreateSettingsOverlayWithSeparateProcess(g_hInstance);
    } else {
        // Normal wallet overlay behavior
        ShowWindow(g_overlay_hwnd, SW_SHOW);
    }
}
```

#### Step 2.2: Create Settings-Specific Overlay Function
**File**: `cef-native/src/handlers/simple_app.cpp`
**Function**: `CreateSettingsOverlayWithSeparateProcess(HINSTANCE hInstance)`
```cpp
void CreateSettingsOverlayWithSeparateProcess(HINSTANCE hInstance) {
    // Create new HWND for settings
    HWND settings_hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"CEFOverlayWindow", nullptr, WS_POPUP | WS_VISIBLE,
        0, 0, width, height, nullptr, nullptr, hInstance, nullptr);

    // Create new CEF browser with subprocess
    CefWindowInfo window_info;
    window_info.SetAsPopup(settings_hwnd, "SettingsOverlay");

    CefBrowserSettings settings;
    settings.windowless_rendering_enabled = true;

    // Create new browser with subprocess
    CefBrowserHost::CreateBrowser(window_info, handler,
                                  "http://localhost:5173/settings", settings);
}
```

#### Step 2.3: Create Settings-Specific React Route
**File**: `frontend/src/App.tsx`
**Route**: Add `/settings` route for settings overlay
**Component**: Create `SettingsOverlayRoot.tsx` for settings-specific state management

#### Step 2.4: Test Settings Button
**Expected Results**:
- ✅ Settings button opens new overlay window
- ✅ Fresh V8 context, no state pollution
- ✅ Settings panel renders correctly
- ✅ Wallet button still works normally

### Phase 3: Implement Process-Per-Overlay for All Overlays
**Goal**: Refactor to support any overlay type with process isolation

#### Step 3.1: Refactor Overlay Creation
**File**: `cef-native/src/handlers/simple_app.cpp`
**Function**: `CreateOverlayWithSeparateProcess(HINSTANCE hInstance, const std::string& overlayType)`
```cpp
void CreateOverlayWithSeparateProcess(HINSTANCE hInstance,
                                     const std::string& overlayType) {
    // Create overlay-specific HWND
    HWND overlay_hwnd = CreateOverlayWindow(overlayType);

    // Create new CEF browser with subprocess
    CefWindowInfo window_info;
    window_info.SetAsPopup(overlay_hwnd, overlayType + "Overlay");

    // Load overlay-specific URL
    std::string url = "http://localhost:5173/" + overlayType;
    CefBrowserHost::CreateBrowser(window_info, handler, url, settings);
}
```

#### Step 3.2: Update Message Handling
**File**: `cef-native/src/handlers/simple_handler.cpp`
**Location**: `OnProcessMessageReceived()` method
```cpp
if (message_name == "overlay_show") {
    std::string overlayType = args->GetString(0); // "wallet" or "settings"
    CreateOverlayWithSeparateProcess(g_hInstance, overlayType);
}
```

#### Step 3.3: Update Frontend API
**File**: `frontend/src/bridge/initWindowBridge.ts`
**Function**: Update `overlay.show()` to accept argument
```typescript
window.bitcoinBrowser.overlay.show = (overlayType: string) => {
    window.cefMessage?.send('overlay_show', [overlayType]);
};
```

### Phase 4: Testing and Validation
**Goal**: Ensure process-per-overlay works correctly for all overlay types

#### Step 4.1: Individual Overlay Tests
- **Settings Overlay**: Fresh state, correct rendering
- **Wallet Overlay**: Fresh state, address generation works
- **Multiple Overlays**: Can open both simultaneously

#### Step 4.2: State Isolation Tests
- **Test**: Open settings, modify state, close, reopen
- **Expected**: Fresh state on reopen
- **Test**: Open wallet, generate address, open settings
- **Expected**: No state interference

#### Step 4.3: Performance Tests
- **Test**: Open/close overlays rapidly
- **Expected**: No memory leaks, clean process cleanup
- **Test**: Multiple overlays open simultaneously
- **Expected**: Stable performance

## 🎯 Simplified Implementation Approach

### The Simplest Method: Argument-Based Process Creation
**Core Concept**: Use a single function `CreateOverlayWithSeparateProcess(overlayType)` that creates a new subprocess for each overlay type.

#### Key Implementation Points:
1. **Single Function**: `CreateOverlayWithSeparateProcess(overlayType)`
2. **Argument-Based**: Pass overlay type as argument ("wallet", "settings")
3. **URL-Based Routing**: Use different URLs for different overlays
4. **Process Isolation**: Each call creates new CEF subprocess

#### Implementation Steps:
1. **Modify `overlay.show()`**: Accept argument: `overlay.show('settings')`
2. **Update C++ Handler**: Create new process based on argument
3. **Create React Routes**: Overlay-specific routes (`/settings`, `/wallet`)
4. **Test Settings Button**: Use as proof-of-concept

### Testing Strategy

#### Phase 1: Settings Button Test
**Goal**: Verify settings button opens dedicated overlay
**Steps**:
1. Click settings button
2. Verify new overlay window opens
3. Verify settings panel renders correctly
4. Verify fresh V8 context (no state pollution)

#### Phase 2: Wallet Button Test
**Goal**: Verify wallet button still works
**Steps**:
1. Click wallet button
2. Verify wallet overlay opens correctly
3. Verify address generation works
4. Verify no interference with settings

#### Phase 3: Multiple Overlays Test
**Goal**: Verify both overlays can be open simultaneously
**Steps**:
1. Open settings overlay
2. Open wallet overlay
3. Verify both work independently
4. Verify no state interference

#### Phase 4: State Isolation Test
**Goal**: Verify fresh state for each overlay
**Steps**:
1. Open settings, modify state, close
2. Reopen settings, verify fresh state
3. Open wallet, generate address
4. Open settings, verify no wallet state interference

### Expected Outcomes

#### After Phase 2 (Settings Button):
- ✅ Settings button opens dedicated settings overlay
- ✅ Fresh V8 context, no state pollution
- ✅ Settings panel renders correctly
- ✅ Wallet button still works normally

#### After Phase 3 (All Overlays):
- ✅ All overlays use process-per-overlay model
- ✅ Complete state isolation between overlays
- ✅ Clean state management for each overlay
- ✅ Better security through process boundaries

#### After Phase 4 (Full Implementation):
- ✅ Process-per-overlay architecture complete
- ✅ Mimics Brave Browser architecture
- ✅ No state pollution issues
- ✅ Clean, maintainable codebase

## 📋 Next Development Priorities

### ✅ Recently Completed (Current Session)
1. **Bitcoin SV Transaction Backend**: ✅ COMPLETE
   - Complete Go daemon with real UTXO fetching from BSV APIs
   - Transaction building with real UTXO selection and fee calculation
   - Multi-miner broadcasting support (TAAL, GorillaPool, Teranode, WhatsOnChain)
   - Security improvements (private key protection, process isolation)
   - Real-world testing with Genesis address and custom addresses
2. **Address Generation Functionality**: ✅ COMPLETE
   - Fixed address generation button getting stuck in "generating" state
   - Implemented proper response handling from Go daemon
   - Address display and clipboard functionality working
   - Frontend now handles address indexing (backend simplified)
3. **Elegant Shutdown Implementation**: ✅ COMPLETE
   - CEF browser process cleanup and destruction implemented
   - HWND window management and proper destruction sequence
   - Go daemon graceful shutdown integration
   - Frontend event listener cleanup in React components
   - Console window cleanup (FreeConsole on shutdown)
4. **Frontend Integration**: ✅ COMPLETE
   - Fixed complete transaction flow from React UI to blockchain
   - Fixed BSV amount conversion (parseInt → parseFloat with multiplication)
   - Fixed transaction ID extraction from GorillaPool responses
   - Fixed balance display with total calculation across all addresses
   - Added missing C++ message handlers for frontend communication
   - Implemented unified `/transaction/send` endpoint
   - Real transaction IDs now displayed in frontend
   - Complete end-to-end testing validated with on-chain transactions

### 🚀 Feature Development (Next Phase)

#### **1. Window Management & UI Improvements** (Priority #1)
**Issues to Address:**
- **Keyboard Commands**: Fix keyboard shortcuts not working in overlays
- **Overlay HWND Movement**: Overlay windows not moving/minimizing/maximizing with main app window
- **Transaction Receipt Display**: Improve UI for transaction confirmation and receipt display
- **Design Aesthetics**: Update visual design and user experience

**Implementation Steps:**
1. **Fix Keyboard Command Handling**
   - Investigate CEF keyboard event handling in overlay processes
   - Implement proper keyboard shortcut routing to overlays
   - Test common shortcuts (Ctrl+C, Ctrl+V, Enter, Escape, etc.)

2. **Fix Overlay Window Management**
   - Implement proper HWND parent-child relationships
   - Add window movement synchronization with main application
   - Implement proper minimize/maximize/restore behavior
   - Test window state changes and focus management

3. **Improve Transaction Receipt Display**
   - Design better transaction confirmation modal
   - Add transaction details (amount, fee, recipient, timestamp)
   - Improve WhatsOnChain link display and styling
   - Add transaction status indicators

4. **Design Aesthetics Updates**
   - Review and improve overall UI/UX design
   - Update color schemes and typography
   - Improve button styles and interactions
   - Add loading states and animations

#### **2. BRC-100 Authentication Integration** (Priority #2)
**Proof-of-Concept Implementation:**

**Overview:**
- Integrate BRC-100 authentication system for Bitcoin SV identity management
- Implement proof-of-concept authentication flow using Bitcoin SV transactions
- Create secure identity verification system for the browser

**Backend (Go Daemon):**
1. **BRC-100 Protocol Implementation**
   - Implement BRC-100 authentication protocol
   - Create identity management endpoints
   - Integrate with existing HD wallet system
   - Create authentication challenge/response system
   - Implement identity verification endpoints

2. **Frontend Integration**
   - Create BRC-100 authentication UI components
   - Implement identity management interface
   - Add authentication status indicators
   - Integrate with existing wallet UI

#### **3. Advanced Address Management** (Priority #3 - Future)
**Deferred to Later Development:**
- Gap limit implementation (20-address standard)
- Address pruning and cleanup
- High-volume address generation
- Privacy-preserving UTXO consolidation
- Address usage tracking and management

**Frontend (React/C++):**
1. **Transaction UI Components** ✅ COMPLETE
   - Transaction form (recipient, amount, fee) ✅ COMPLETE
   - Transaction confirmation modals ✅ COMPLETE
   - Transaction status indicators ✅ COMPLETE
   - WhatsOnChain integration ✅ COMPLETE

2. **C++ Integration** ✅ COMPLETE
   - Transaction message handlers in `simple_handler.cpp` ✅ COMPLETE
   - V8 bindings for transaction operations ✅ COMPLETE
   - Error handling and user feedback ✅ COMPLETE

#### **4. Transaction History** (Priority #4 - Future)
**Implementation Steps:**
1. **Transaction Storage**
   - Add transaction history storage to wallet.json
   - Implement transaction categorization and filtering
   - Add search and export functionality

2. **Transaction Display**
   - Create transaction history UI components
   - Implement transaction details view
   - Add transaction status tracking

---

## 🏗️ **Advanced Wallet Management Architecture** (Future Planning)

### **📋 Features Requiring Architectural Analysis**

These features are important but require careful architectural planning before implementation. Each needs a comprehensive design analysis to ensure proper integration with the existing system.

#### **1. Multiple Wallet Support**
**Status:** Requires architectural analysis
**Complexity:** High - affects core identity system, file management, and UI

**Key Design Questions:**
- How to modify existing single-identity system to support multiple wallets?
- File structure and storage strategy (`~/AppData/Roaming/BabbageBrowser/wallets/`)
- Wallet selection and switching mechanisms
- Address generation per wallet vs shared address space
- Transaction history organization and filtering
- Security isolation between wallets
- Import/export functionality and standards

**Implementation Considerations:**
- Breaking changes to existing identity system
- Database vs file-based wallet storage
- UI/UX for wallet management (creation, deletion, switching)
- Backup and recovery per wallet vs global
- Performance implications of multiple wallet scanning

#### **2. Wallet Recovery System**
**Status:** Requires architectural analysis
**Complexity:** High - security and cryptographic considerations

**Key Design Questions:**
- Recovery method options (seed phrases, private key backup, hardware wallet)
- BIP39/BIP44 mnemonic implementation and standards
- Recovery phrase generation, storage, and validation
- Import existing wallets from other software
- Recovery verification and testing procedures
- Security considerations for recovery data storage

**Implementation Considerations:**
- Cryptographic libraries and dependencies
- Recovery phrase UI/UX (security best practices)
- Integration with existing identity system
- Backup file formats and standards
- Cross-platform recovery compatibility

#### **3. Advanced Security Features**
**Status:** Requires architectural analysis
**Complexity:** High - security-critical implementation

**Key Design Questions:**
- Hardware wallet integration
- Multi-signature wallet support
- Advanced key derivation (PBKDF2, scrypt, Argon2)
- Encrypted private key storage
- Secure communication protocols
- Biometric authentication integration

**Implementation Considerations:**
- Hardware wallet communication protocols
- Security audit requirements
- Key management and storage strategies
- User experience for security features
- Compliance and regulatory considerations

#### **4. Advanced Transaction Features**
**Status:** Requires architectural analysis
**Complexity:** Medium-High - Bitcoin protocol complexity

**Key Design Questions:**
- Transaction batching and optimization
- Advanced fee estimation algorithms
- Atomic swaps and cross-chain transactions
- Transaction privacy features

**Implementation Considerations:**
- Bitcoin protocol implementation complexity
- Network integration and SPV considerations
- User interface for advanced features
- Performance and scalability implications

### **📅 When to Analyze These Features**

**Recommended Timeline:**
1. **After Core Functionality** - Complete transaction signing/broadcasting first
2. **Before Major UI Overhaul** - Plan wallet management before redesigning interface
3. **Security Audit Phase** - Analyze security features during security review
4. **Performance Optimization** - Consider advanced features during performance tuning

**Analysis Process:**
1. **Research Phase** - Study existing wallet implementations and standards
2. **Architecture Design** - Create detailed technical specifications
3. **Prototype Development** - Build proof-of-concepts for complex features
4. **Security Review** - Audit security-critical features before implementation
5. **User Testing** - Validate UX decisions with real users

**Decision Factors:**
- User demand and feedback
- Technical feasibility and complexity
- Security implications and audit requirements
- Integration with existing architecture
- Development timeline and resource availability

---

## 🔒 **Security Enhancements** (Future)
- Implement proper key derivation (PBKDF2)
- Add encryption for private key storage
- Secure daemon communication
- Process isolation improvements

### 🎯 Long-term Goals
1. **Rust Migration**: Plan migration path from Go to Rust
2. **Mobile Support**: React Native implementation
3. **Multi-platform**: Windows, Mac, Linux builds
4. **Advanced Features**: Hardware wallet support, BRC-100 full implementation

### ✅ Completed Integration
- **Complete Pipeline**: React → C++ → Go → Response ✅
- **Process Management**: Automatic daemon startup/monitoring ✅
- **HTTP Communication**: Windows HTTP API client ✅
- **Identity Management**: Full CRUD operations ✅
- **Error Handling**: Robust error recovery ✅
- **Process-Per-Overlay Architecture**: Complete implementation ✅
- **Identity System Rebuild**: Complete rewrite with proper file management ✅
- **Backup Modal System**: Full identity creation and backup flow ✅
- **Settings Overlay**: Dedicated process with proper window management ✅
- **Wallet Overlay**: Isolated process with fresh V8 context ✅
- **Address Generation**: Complete functionality with proper indexing ✅
- **Graceful Shutdown**: Complete CEF browser, HWND, and daemon cleanup ✅
- **Console Management**: Proper console window lifecycle management ✅
- **Bitcoin SV Transaction Backend**: Complete Go daemon with real UTXO fetching ✅
- **Multi-Miner Broadcasting**: Support for TAAL, GorillaPool, Teranode, WhatsOnChain ✅
- **Transaction Building**: Real UTXO selection and fee calculation ✅
- **Security Improvements**: Private key protection and process isolation ✅

---

## 🎨 **WALLET UI/UX DESIGN VISION**

### **Design Philosophy: Physical Wallet Metaphor**
Inspired by 90s desktop metaphors (desktop, files, folders), the wallet should feel like a **real physical wallet** to help users understand and relate to digital money management.

#### **Physical Wallet Elements:**
- **Cash/Credit Card**: BSV balance display (primary money)
- **Driver's License**: Identity management and verification
- **Rewards Cards**: Token management and loyalty programs
- **Photos**: Personal tokens and sentimental digital assets
- **Receipts**: Transaction history and records

### **Core UI/UX Principles:**

#### **1. Simplified Transaction Flow**
- **Single-Click Send**: Click "Send" → Form appears → Fill → Confirm → Done
- **No Multi-Step Process**: Eliminate separate create/sign/broadcast steps
- **One-Click Confirmation**: Create, sign, and broadcast in single action
- **Smart Defaults**: Pre-filled reasonable values (fees, amounts)

#### **2. Currency Toggle System**
- **BSV/USD Toggle**: Switch between BSV and USD input modes
- **Real-Time Conversion**: Automatic satoshi calculation based on current price
- **Visual Clarity**: Clear indication of which currency mode is active
- **Smart Rounding**: User-friendly rounding for display values

#### **3. Receive Functionality**
- **Receive Button**: Replace "Generate Address" with "Receive" button
- **Auto-Generate**: New address generated on each click
- **Auto-Copy**: Address automatically copied to clipboard
- **Visual Display**: Show address for verification before copying

#### **4. Clean Interface Design**
- **No Tabs**: Remove current tab system (Dashboard/Send)
- **Single View**: All functionality accessible from main wallet view
- **Minimalist**: Clean, uncluttered interface
- **Focus on Actions**: Primary actions (Send/Receive) prominently displayed

#### **5. Transaction Feedback**
- **Success Alerts**: Clear confirmation when transactions complete
- **WhatsOnChain Links**: Direct links to view transactions on blockchain
- **Status Indicators**: Visual feedback for transaction states
- **Error Handling**: Clear, actionable error messages

#### **6. Future Features (Not Priority)**
- **Paymail Integration**: Simplified address input using paymail
- **Auto-Pay**: Micro-transaction approval for small amounts
- **Exchange Integration**: Built-in exchange functionality (long-term)

### **Design Implementation Strategy:**

#### **Phase 1: Core Interface Redesign**
1. **Remove Tab System**: Eliminate Dashboard/Send tabs
2. **Implement Send Flow**: Single-click send with integrated form
3. **Add Receive Functionality**: Receive button with auto-copy
4. **Currency Toggle**: BSV/USD input switching
5. **Transaction Integration**: Complete create/sign/broadcast flow

#### **Phase 2: Enhanced UX**
1. **Physical Wallet Styling**: Implement wallet-like visual design
2. **Transaction Feedback**: Success/error alerts and status indicators
3. **WhatsOnChain Integration**: Transaction viewing links
4. **Auto-Pay Foundation**: Basic micro-transaction handling

#### **Phase 3: Advanced Features**
1. **Paymail Support**: Simplified address management
2. **Auto-Pay System**: Configurable micro-transaction approvals
3. **Exchange Integration**: Built-in trading functionality

## 🎯 **NEXT SESSION PRIORITIES**

### **Phase 1: Complete Transaction System (Immediate)**
1. **Real Transaction Serialization**
   - Replace placeholder serialization with proper Bitcoin transaction format
   - Use `bitcoin-sv/go-sdk` for transaction building
   - Test with real transaction data

2. **Real Transaction Signing**
   - Replace placeholder signing with real cryptographic operations
   - Use `bitcoin-sv/go-sdk` for signature generation
   - Test with real private keys

3. **Frontend Transaction UI**
   - Create React transaction form components
   - Add C++ message handlers for transaction operations
   - Integrate with existing wallet overlay

### **Phase 2: Enhanced Features (Next)**
1. **Balance Display in USD**
   - Price API integration (CoinGecko/CoinMarketCap)
   - Real-time balance calculation
   - USD conversion display

2. **Transaction History**
   - Display past transactions
   - Status tracking and confirmation
   - Transaction details and metadata

### **Phase 3: Advanced Features (Future)**
1. **HD Wallet Support**
   - BIP32/BIP44 implementation
   - Multiple address generation
   - Seed phrase backup and recovery

2. **Advanced Security**
   - Hardware wallet integration
   - Multi-signature support
   - Enhanced key management

### **Current Status Summary:**
- ✅ **Architecture**: Process-per-overlay system complete
- ✅ **HD Wallet**: BIP44 hierarchical deterministic wallet implementation complete
- ✅ **Backend**: Bitcoin SV transaction system complete
- ✅ **Transaction Creation**: Working with real UTXO data
- ❌ **Transaction Signing**: Not yet tested with real transactions
- ❌ **Transaction Broadcasting**: Not yet tested with real transactions
- ❌ **Frontend Integration**: Not yet integrated with React UI
- 🔄 **Next**: Test transaction signing, broadcasting, and frontend integration
- 🎯 **Goal**: Complete end-to-end Bitcoin SV transaction system

---

## 🏭 Production Build Configuration

### 🚨 **CRITICAL: Context Menu Security Strategy**

**YES, you MUST remove/disable the context menu in production builds.** This is essential for security and user experience.

#### **Why Remove Context Menu in Production:**

1. **Security**: Prevents users from accessing DevTools and potentially inspecting sensitive code
2. **User Experience**: Cleaner, more professional appearance
3. **Performance**: Slight performance improvement
4. **Control**: Prevents users from accessing developer features

#### **Implementation Strategy:**

**Recommended Approach - Build Configuration System:**

```cpp
// In simple_handler.cpp - OnBeforeContextMenu method
void SimpleHandler::OnBeforeContextMenu(...) {
    #ifdef DEBUG_BUILD
        // Enable DevTools for settings overlay in development
        if (role_ == "settings") {
            model->AddItem(MENU_ID_DEV_TOOLS, "Inspect Element");
            model->AddSeparator();
        }
    #else
        // Production build - disable context menu entirely
        model->Clear();
    #endif
}
```

#### **Alternative Implementation Methods:**

1. **Role-based Control**:
   ```cpp
   if (role_ == "settings" && isDevelopmentBuild) {
       // Add DevTools
   }
   ```

2. **Command Line Flag**:
   ```cpp
   // Check for --enable-devtools flag
   if (commandLine->HasSwitch("enable-devtools")) {
       // Enable context menu
   }
   ```

3. **Environment Variable**:
   ```cpp
   // Check environment variable
   if (getenv("BITCOIN_BROWSER_DEBUG")) {
       // Enable DevTools
   }
   ```

#### **Production Browser Settings:**

```cpp
// In production builds - secure configuration
CefBrowserSettings settings;
settings.javascript_access_clipboard = STATE_DISABLED;
settings.plugins = STATE_DISABLED;
settings.web_security = STATE_ENABLED; // Re-enable security
settings.javascript = STATE_ENABLED; // Keep JS enabled for functionality
// No context menu handler or empty context menu
```

#### **Build Configuration Checklist:**

- [ ] **Development Build**: Context menu with DevTools enabled
- [ ] **Production Build**: Context menu completely disabled
- [ ] **Security Settings**: Web security enabled, plugins disabled
- [ ] **Performance**: Optimized settings for production
- [ ] **Testing**: Verify context menu behavior in both builds

#### **Security Considerations:**

- **Never ship with DevTools enabled** in production
- **Test production builds thoroughly** to ensure no developer features leak through
- **Consider code obfuscation** for additional protection
- **Implement proper error handling** that doesn't expose internal details

#### **Identity Check Migration Note:**

**IMPORTANT**: The current identity backup check that runs on every app launch is a temporary development feature. In production:

- **Identity backup verification will be moved to the installation/setup process**
- **The backup modal will NOT run on every app launch**
- **Users will complete wallet backup during initial setup, not during normal usage**
- **This eliminates the need for the backup modal overlay system in production builds**

This change will significantly improve app startup performance and user experience in production.

#### **CRITICAL ISSUE: Backup Modal Rendering - EXTENSIVE DEBUGGING SESSION**

**Current Status**: Backup modal overlay window opens but JSX content doesn't display in the HWND, despite React components rendering correctly.

**Technical Details**:
- ✅ React components work perfectly - BackupOverlayRoot and BackupModal render correctly
- ✅ Identity data loads and displays properly in console logs
- ✅ CEF browser is created and loads the `/backup` route successfully
- ❌ CEF OnPaint method is never called, so content isn't painted to the HWND
- ❌ Force repaint messages don't reach the C++ handler

**Root Cause**: CEF overlay state corruption between runs. This is a deep CEF internals issue with overlay HWND rendering, not a React or frontend issue.

**Time Spent**: Hundreds of hours debugging this specific issue across multiple sessions.

**Attempted Solutions (All Failed)**:
1. **C++ Backend State Management**: 5-phase implementation with persistent state
2. **localStorage Persistence**: Frontend state persistence across remounts
3. **Ready State Pattern**: Synchronized startup sequence
4. **Visual Debugging**: Bright colors to isolate rendering issues
5. **Complete Overlay Recreation**: Force new HWND creation each time
6. **Memory Management**: Extensive memory leak debugging
7. **Timing Synchronization**: Multiple timing fixes and delays
8. **Web Research**: Extensive research on CEF + React state reset issues

**Research Findings**:
- Common issue with CEF + React state reset in overlay windows
- Memory leaks in CEF offscreen mode can cause rendering corruption
- Process isolation issues between CEF subprocesses
- HWND state corruption between runs

**Conclusion**: This is a fundamental CEF architecture issue that would require deep CEF internals debugging to resolve.

**Workaround**: This functionality will be removed in production anyway (moved to installation process), so this is not blocking core development.

**Recommendation**: Skip this issue and focus on core wallet functionality. The backup modal can be implemented as part of the installation/setup process instead of the main application startup.

## Previous Session: Backup Modal Overlay HWND Issue

### Problem Statement
The backup modal needs to render in an `overlay_hwnd` at application startup when the identity file doesn't exist or its `backedUp` field is `false`. The modal should be populated with identity data, and checking a box should update the `backedUp` field to `true` and close the modal.

### Architecture Overview

#### HWND Structure
- **`g_hwnd`**: Main shell window (parent)
- **`g_header_hwnd`**: React UI (header controls)
- **`g_webview_hwnd`**: Web content display
- **`g_overlay_hwnd`**: Dynamic overlay for panels/modals (created as needed)

#### Communication Flow
1. **Frontend → Backend**: CEF messages via `window.cefMessage.send()`
   - `"overlay_open_panel"`: Opens a panel in overlay
   - `"overlay_show"`: Shows overlay window
   - `"overlay_hide"`: Hides overlay window
   - `"overlay_input"`: Toggles mouse input

2. **Backend → Frontend**: V8 JavaScript injections
   - `window.triggerPanel(panelName)`: Triggers React panel rendering
   - `window.bitcoinBrowser.identity.get()`: Gets identity data
   - `window.bitcoinBrowser.overlayPanel.open(panelName)`: Opens panel

#### Key Files
- **`cef-native/src/handlers/simple_app.cpp`**: Creates overlay HWND
- **`cef-native/src/handlers/simple_handler.cpp`**: Handles CEF messages and browser lifecycle
- **`frontend/src/App.tsx`**: Main React app with identity check logic
- **`frontend/src/pages/OverlayRoot.tsx`**: Overlay React component
- **`frontend/src/components/panels/BackupModal.tsx`**: Backup modal component

### Current State

#### What Works
1. **Identity Creation**: Backend creates identity file correctly
2. **Overlay HWND Creation**: `CreateOverlayBrowserIfNeeded()` creates overlay window
3. **Wallet Panel**: Opens and closes correctly in overlay
4. **Deferred Trigger Fix**: Fixed duplicate deferred triggers issue

#### What's Broken
1. **Backup Modal Rendering**: When overlay HWND is reused, backup modal doesn't render
2. **Overlay Closing**: Without HWND reuse check, overlay only closes once

### Critical Code Changes Made

#### 1. Overlay HWND Reuse Check (FIXES CLOSING ISSUE)
**File**: `cef-native/src/handlers/simple_app.cpp`
**Location**: `CreateOverlayBrowserIfNeeded()` function
```cpp
if (g_overlay_hwnd && IsWindow(g_overlay_hwnd)) {
    std::cout << "🔄 Reusing existing overlay HWND: " << g_overlay_hwnd << std::endl;
    return; // Exit early, don't create new one
}
```
**Effect**:
- ✅ **FIXES**: Overlay can be opened/closed multiple times
- ❌ **BREAKS**: Backup modal doesn't render when overlay is reused

#### 2. Deferred Trigger Fix (FIXES EMPTY STRING ISSUE)
**File**: `cef-native/src/handlers/simple_handler.cpp`
**Location**: `OnLoadingStateChange()` method
```cpp
if (!isLoading && role_ == "overlay" && !pending_panel_.empty()) {
    std::string panel = pending_panel_;
    std::cout << "🕒 OnLoadingStateChange: Creating deferred trigger for panel: " << panel << std::endl;

    // Clear pending_panel_ immediately to prevent duplicate deferred triggers
    SimpleHandler::pending_panel_.clear();

    // Store panel name in static variable for deferred trigger
    static std::string deferred_panel = panel;

    // Delay JS execution slightly to ensure React is mounted
    CefPostDelayedTask(TID_UI, base::BindOnce([]() {
        CefRefPtr<CefBrowser> overlay = SimpleHandler::GetOverlayBrowser();
        if (overlay && overlay->GetMainFrame()) {
            std::string js = "window.triggerPanel('" + deferred_panel + "')";
            overlay->GetMainFrame()->ExecuteJavaScript(js, overlay->GetMainFrame()->GetURL(), 0);
            std::cout << "🧠 Deferred panel triggered after delay: " << deferred_panel << std::endl;
        } else {
            std::cout << "⚠️ Overlay browser still not ready. Skipping panel trigger." << std::endl;
        }
    }), 100);
}
```
**Effect**:
- ✅ **FIXES**: Prevents empty string in deferred triggers
- ✅ **WORKS**: Only one deferred trigger fires with correct panel name

#### 3. Pending Panel Guard (PREVENTS OVERWRITES)
**File**: `cef-native/src/handlers/simple_handler.cpp`
**Location**: `OnProcessMessageReceived()` method
```cpp
else {
    std::cout << "🕒 Deferring overlay panel trigger until browser is ready: " << panel << std::endl;
    // Only set pending_panel_ if it's empty (prevent overwrites)
    if (SimpleHandler::pending_panel_.empty()) {
        SimpleHandler::pending_panel_ = panel;
    } else {
        std::cout << "🕒 Skipping deferred trigger - already pending: " << SimpleHandler::pending_panel_ << std::endl;
    }
}
```
**Effect**:
- ✅ **FIXES**: Prevents `pending_panel_` from being overwritten by duplicate messages

### Debugging Findings

#### Root Cause Analysis
1. **React StrictMode**: Causes `useEffect` to run twice, sending duplicate `overlay_open_panel` messages
2. **Static Variable Issue**: `SimpleHandler::pending_panel_` gets overwritten by duplicate messages
3. **Deferred Trigger Problem**: Multiple `OnLoadingStateChange` calls create multiple deferred triggers
4. **HWND Reuse Issue**: When overlay HWND is reused, React app state isn't properly reset

#### What We Tried (All Failed)
1. **Reloading React App**: `existing_overlay->GetMainFrame()->LoadURL("http://127.0.0.1:5137/overlay")`
   - **Result**: Breaks overlay window completely
   - **Issue**: Causes infinite loading cycles and window corruption

2. **Lambda Capture**: Using `[panel]()` in `CefPostDelayedTask`
   - **Result**: Build errors - CEF doesn't support C++11 lambda capture
   - **Error**: `base::BindOnce` template instantiation failures

3. **Frontend Guards**: Using `useRef` or `useState` to prevent duplicate calls
   - **Result**: Doesn't fix backend duplicate message issue
   - **Issue**: Problem is in CEF message handling, not React

#### Current Log Analysis
```
🕒 Deferring overlay panel trigger until browser is ready: backup
🔄 Reusing existing overlay HWND: 0000000000F70BF2
🔄 Current overlay URL:  ← EMPTY URL!
🔄 Loading React app in existing overlay
🎯 Deferred panel triggered after delay: backup
```
**Key Finding**: The overlay browser has an **empty URL** when reused, indicating the React app isn't loaded.

### Next Steps

#### Immediate Priority
1. **Fix React App Loading**: Ensure React app loads properly when overlay HWND is reused
2. **Debug URL Issue**: Why is the overlay browser URL empty when reused?
3. **Test Modal Rendering**: Verify backup modal renders after fixing React loading

#### Potential Solutions to Try
1. **Check Browser State**: Verify overlay browser is properly initialized when reused
2. **Force URL Load**: Ensure overlay browser loads the correct URL on reuse
3. **Reset Browser Context**: Clear browser state when reusing overlay HWND
4. **Alternative Approach**: Don't reuse overlay HWND, create new one each time (but fix closing issue differently)

#### Files to Investigate
- **`cef-native/src/handlers/simple_handler.cpp`**: Browser lifecycle management
- **`cef-native/src/handlers/simple_app.cpp`**: Overlay HWND creation logic
- **`frontend/src/pages/OverlayRoot.tsx`**: React component state management

### Technical Notes

#### CEF Message Types
- **CEF Messages**: `window.cefMessage.send()` → `OnProcessMessageReceived()`
- **V8 Injections**: `ExecuteJavaScript()` → Direct function calls in render process

#### React StrictMode Impact
- **Development**: Double-renders components and effects
- **Production**: No impact
- **Current**: Disabled to reduce debugging complexity

#### Static Variable Issues
- **Problem**: `SimpleHandler::pending_panel_` shared across all instances
- **Solution**: Guard against overwrites, clear immediately after use
- **Alternative**: Use instance variables instead of static

### Session Summary
- **Started**: Backup modal not rendering in overlay HWND
- **Progress**: Fixed duplicate deferred triggers, overlay closing issue
- **Current**: Overlay HWND reuse breaks React app loading
- **Next**: Fix React app loading when reusing overlay HWND

---

## 🎉 **TRANSACTION SYSTEM COMPLETE - SESSION SUMMARY**

### **What We Accomplished:**

**✅ COMPLETE TRANSACTION FLOW IMPLEMENTED:**
1. **Transaction Creation**: Builds transactions with real UTXO data using BSV SDK
2. **Transaction Signing**: Signs transactions with proper source transactions and unlocking scripts
3. **Transaction Broadcasting**: Successfully broadcasts to multiple BSV miners
4. **On-Chain Validation**: Transactions confirmed on-chain with valid transaction IDs

**✅ KEY TECHNICAL BREAKTHROUGHS:**
1. **BSV SDK Integration**: Proper use of `go-sdk` for transaction operations
2. **Source Transaction Handling**: Fetching and storing source transactions for signing
3. **Unlocking Script Templates**: Using `p2pkh.Unlock()` for proper script templates
4. **Transaction Object Persistence**: Storing transaction objects instead of parsing from hex
5. **Multi-Miner Broadcasting**: Support for WhatsOnChain and GorillaPool
6. **Rate Limiting Handling**: Retry logic for API rate limits

**✅ TESTING VALIDATION:**
- **End-to-End Testing**: Complete transaction flow tested and validated
- **On-Chain Confirmation**: Transactions confirmed on BSV blockchain
- **Example Transaction**: `38ac3f46a5e1d5c79dc0e697f15f6bf1cc4cb0114877b8e01c2690a8aac361e5`

### **What We Learned:**

**🔍 BSV SDK Requirements:**
- `tx.Sign()` requires source transactions and unlocking script templates
- Parsing transactions from hex loses critical signing context
- Must store transaction objects with source transactions for proper signing

**🔍 Transaction Signing Process:**
1. Fetch source transaction for each UTXO
2. Create unlocking script template using `p2pkh.Unlock()`
3. Store transaction object with source transactions
4. Call `tx.Sign()` with proper context
5. Broadcast signed transaction to miners

**🔍 Broadcasting Requirements:**
- Different miners require different payload formats
- Rate limiting must be handled with retry logic
- Response parsing varies by miner type

### **Next Steps - Frontend Integration:**

**🎯 IMMEDIATE PRIORITIES:**
1. **TransactionForm.tsx Integration**: Connect React UI to Go daemon endpoints
2. **useTransaction.ts Hook**: Implement transaction management functions
3. **useBalance.ts Updates**: Add balance refresh after transactions
4. **Transaction History**: Display recent transactions in wallet panel
5. **Error Handling**: Comprehensive error handling and user feedback

**🎯 FRONTEND INTEGRATION TASKS:**
- Connect transaction form to `/transaction/create` endpoint
- Handle transaction creation, signing, and broadcasting flow
- Show loading states and success/error messages
- Update balances after successful transactions
- Display transaction history with status and details

The backend transaction system is now complete and ready for frontend integration!

---

## 🔐 **ADDRESS MANAGEMENT ANALYSIS & BEST PRACTICES**

### **Current Implementation Status:**

**✅ ADDRESS GENERATION FLOW:**
1. **Frontend**: `handleReceiveClick()` → `generateAndCopy()`
2. **Hook**: `useAddress.ts` → `window.bitcoinBrowser.address.generate()`
3. **C++ Bridge**: `WalletService.cpp` → `POST /wallet/address/generate`
4. **Go Daemon**: `GetNextAddress()` → saves to `wallet.json`

**✅ CURRENT BEHAVIOR:**
- **New address every click**: Each "Receive" generates a new address
- **Persistent storage**: All addresses saved to `wallet.json`
- **No limits**: Unlimited address generation
- **Sequential indexing**: `CurrentIndex` tracks next address

### **Industry Standards & Best Practices:**

**🔍 Gap Limit (BIP44 Standard):**
- **Standard**: 20 unused addresses maximum
- **Purpose**: Prevents infinite address scanning during wallet recovery
- **Implementation**: Stop scanning after 20 consecutive unused addresses
- **Recovery**: Wallets scan addresses until gap limit reached

**🔍 Address Management Best Practices:**
1. **Avoid address reuse**: Generate new address for each transaction
2. **Use change addresses**: Send change to new addresses for privacy
3. **Implement gap limit**: 20-address limit for wallet recovery
4. **Mark addresses as used**: Track which addresses have received funds
5. **Automatic discovery**: Scan blockchain for used addresses during recovery

### **Security & Privacy Implications:**

**🔒 Current Risks:**
- **No address limit**: Could generate infinite addresses, making recovery difficult
- **Missing gap limit**: Wallet recovery might miss addresses beyond gap
- **No pruning**: Unused addresses accumulate, increasing attack surface
- **Address reuse**: Current implementation doesn't prevent reuse
- **No change addresses**: Transactions don't use fresh change addresses

**🔒 High-Volume Scenarios (20+ addresses/minute):**
- **Privacy risk**: Rapid address generation can create patterns
- **Tracking risk**: Addresses generated in quick succession may be linked
- **Solution**: Implement address pooling and staggered generation

### **Recommended Implementation Strategy:**

**🎯 Phase 1: Basic Gap Limit (Proof-of-Concept)**
```go
const GAP_LIMIT = 20
const MAX_ADDRESSES = 1000 // Reasonable limit for testing

func (wm *WalletManager) GetNextAddress() (*AddressInfo, error) {
    // Check if we've hit the address limit
    if len(wm.wallet.Addresses) >= MAX_ADDRESSES {
        return nil, fmt.Errorf("address limit reached")
    }

    // Generate next address
    address, err := wm.GenerateAddress(wm.wallet.CurrentIndex)
    if err != nil {
        return nil, err
    }

    // Add to wallet
    wm.wallet.Addresses = append(wm.wallet.Addresses, *address)
    wm.wallet.CurrentIndex++

    return address, nil
}
```

**🎯 Phase 2: Address Usage Tracking**
```go
func (wm *WalletManager) MarkAddressUsed(address string) error {
    for i := range wm.wallet.Addresses {
        if wm.wallet.Addresses[i].Address == address {
            wm.wallet.Addresses[i].Used = true
            wm.wallet.Addresses[i].LastUsed = time.Now()
            return wm.SaveToFile(GetWalletPath())
        }
    }
    return fmt.Errorf("address not found")
}
```

**🎯 Phase 3: UTXO Consolidation (Privacy-Preserving)**
```go
func (wm *WalletManager) ConsolidateUTXOs() error {
    // Find addresses with small UTXOs
    smallUTXOAddresses := wm.findAddressesWithSmallUTXOs()

    // Create consolidation transaction
    // Send all small UTXOs to a new address
    // Use multiple intermediate addresses to break links

    return nil
}
```

### **Testing Strategy:**

**🔍 Address Tracking:**
1. **Counter**: Log every address generation with timestamp
2. **UTXO monitoring**: Track which addresses receive funds
3. **Balance verification**: Ensure total balance = sum of address balances
4. **Gap limit testing**: Verify recovery works within 20-address gap

**🔍 Privacy Testing:**
1. **Pattern detection**: Ensure no predictable address generation patterns
2. **Linkability testing**: Verify addresses can't be easily linked
3. **Consolidation testing**: Test UTXO consolidation doesn't create patterns

### **Immediate Recommendations:**

**For Proof-of-Concept:**
1. **Keep current unlimited generation** (simple, works for testing)
2. **Add address usage tracking** (mark addresses as used when they receive funds)
3. **Implement basic address limit** (1000 addresses max for testing)
4. **Add testing counters** (log address generation and usage)

**For High-Volume Scenarios:**
1. **Address pooling**: Pre-generate addresses in batches
2. **Staggered generation**: Add random delays between address generation
3. **UTXO consolidation**: Regularly consolidate small UTXOs
4. **Privacy mixing**: Use intermediate addresses to break transaction links

### **Address Management Roadmap:**

**🎯 Next Development Priorities:**
1. **Frontend Transaction Integration**: Connect React UI to Go daemon endpoints
2. **Address Usage Tracking**: Mark addresses as used when they receive funds
3. **Basic Gap Limit**: Implement 20-address gap limit for recovery
4. **UTXO Consolidation**: Privacy-preserving consolidation for high-volume scenarios
5. **Address Pooling**: Pre-generate addresses for high-volume use cases
