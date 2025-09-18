# Developer Notes - Bitcoin Browser

## 🚨 **CURRENT SESSION STATUS - READ THIS FIRST**

### **Current State: Settings Overlay Implementation - PARTIALLY WORKING**

**✅ WHAT'S WORKING:**
1. **Settings Button**: Opens settings overlay window in separate process
2. **Process Isolation**: Settings overlay runs in its own CEF subprocess (process-per-overlay architecture)
3. **Window Creation**: Settings overlay window is created with dedicated `CEFSettingsOverlayWindow` class
4. **API Injection**: `bitcoinBrowser.overlay.close()` method is properly injected into settings overlay
5. **Message Handling**: `overlay_close` message is sent from frontend to native C++

**❌ CURRENT ISSUE:**
- **Settings overlay window does not close** when clicking the close button
- The window is created and renders correctly, but `DestroyWindow()` call isn't working

### **Architecture Overview:**
- **Main Browser**: React app running on `http://127.0.0.1:5137` (header browser)
- **Settings Overlay**: Separate CEF process running on `http://127.0.0.1:5137/settings`
- **Communication**: Uses `cefMessage.send()` for IPC between processes
- **Window Management**: Each overlay has its own HWND with dedicated message handlers

### **Key Files Modified:**
1. **`cef-native/src/handlers/simple_handler.cpp`**:
   - Added `overlay_show_settings` handler
   - Added `overlay_close` handler (currently using `DestroyWindow()`)
2. **`cef-native/src/handlers/simple_app.cpp`**:
   - Added `CreateSettingsOverlayWithSeparateProcess()` function
3. **`cef-native/cef_browser_shell.cpp`**:
   - Added `SettingsOverlayWndProc` for mouse input handling
4. **`frontend/src/pages/SettingsOverlayRoot.tsx`**:
   - Settings overlay React component
5. **`frontend/src/pages/MainBrowserView.tsx`**:
   - Settings button handler

### **Current Problem:**
The settings overlay window is created successfully but won't close. The `overlay_close` message is being sent and received, but `DestroyWindow(settings_hwnd)` isn't working as expected.

### **Next Steps Needed:**
1. **Debug the close functionality** - investigate why `DestroyWindow()` isn't working
2. **Test mouse input** - ensure left-click and right-click work on settings overlay
3. **Verify process cleanup** - ensure CEF subprocess is properly terminated when window closes

### **Technical Context:**
- **CEF Version**: Using CEF binaries with process-per-overlay architecture
- **React**: Frontend running on Vite dev server
- **Build System**: CMake with Visual Studio
- **Debug Logging**: All native logs go to `debug_output.log`

---

## 🎯 Current Development Focus: Process-Per-Overlay Architecture Implementation

### 🚀 NEW FOCUS: Process-Per-Overlay Model (Brave Browser Style)
- **Status**: Planning and implementation phase
- **Goal**: Implement process-per-overlay architecture for better security and state isolation
- **Current Issue**: Settings button opens JSX in header instead of overlay
- **Target**: Each overlay (wallet, settings) runs in its own CEF subprocess

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

### 🔧 Immediate Fixes (Next Session)
1. **Process-Per-Overlay Implementation**: Start with settings button
   - Implement `CreateSettingsOverlayWithSeparateProcess()`
   - Create settings-specific React route
   - Test settings button opens overlay correctly
2. **Console Shutdown Issue**: Fix console window not responding to X button
   - Add Console Control Handler for graceful shutdown
   - Ensure daemon process cleanup on app exit
   - Improve user experience for development

### 🚀 Feature Development (Next Phase)
1. **Enhanced Wallet Features**:
   - Transaction signing and broadcasting
   - BRC-100 authentication implementation
   - SPV transaction verification
   - Multiple wallet support

2. **UI/UX Improvements**:
   - Fix overlay HWND React loading issues
   - Improve backup modal functionality
   - Add wallet status indicators
   - Better error messaging

3. **Security Enhancements**:
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
