# Developer Notes - Babbage Browser

## 🎯 Current Development Focus: C++ CEF Integration with Go Wallet Daemon

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
