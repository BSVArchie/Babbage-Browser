# Systematic Migration Plan: Multi-Process Overlays → Single-Process Integrated Browser

## 🎯 **Migration Overview**

**Goal**: Transform from multi-process overlay architecture to single-process integrated browser with React panels and embedded CefBrowserView, following modern browser architecture patterns.

**Current Architecture**:
- Multiple CEF processes (main + overlays)
- Separate HWNDs for each overlay
- Separate webview_hwnd for browsing

**Target Architecture**:
- Single CEF process with integrated components
- CefBrowserView embedded in header_hwnd
- React panels sliding over webview area

### **Critical Architecture Requirements:**
- **Single Top-Level Window**: Main HWND only - no separate CefWindow instances
- **CefBrowserView Integration**: Must be embedded as child of header_hwnd, not separate window
- **Space Management**: Panels and webview share same space with proper z-order management
- **Fully Functional Browser**: Complete web browsing capabilities within integrated layout

---

## 📊 **Microservices Breakdown**

### **Frontend (React + Vite)**
- **Current**: Multiple overlay processes with separate routing
- **Target**: Single main process with integrated panel management
- **Changes**: Layout restructuring, state management, panel positioning

### **C++ Backend (CEF + Native)**
- **Current**: Multiple CEF processes, complex HWND management
- **Target**: Single CEF process with simplified window management
- **Changes**: Remove overlay HWND creation, consolidate messaging

### **Wallet Daemon (Go)**
- **Current**: HTTP API endpoints working correctly
- **Target**: No changes required
- **Changes**: None - all existing functionality preserved

---

## 🗓️ **Phase-by-Phase Migration Plan**

### **Phase 0: Code Preservation & Reference Setup** ⏱️ *0.5 days*

#### **C++ Backend Tasks:**
- [ ] **Comment out overlay creation functions**
  - [ ] `CreateSettingsOverlayWithSeparateProcess()`
  - [ ] `CreateWalletOverlayWithSeparateProcess()`
  - [ ] `CreateBackupOverlayWithSeparateProcess()`
  - [ ] Add `// TODO: REMOVE AFTER MIGRATION` comments with clear descriptions

- [ ] **Comment out webview_hwnd creation**
  - [ ] Comment out `webview_hwnd` creation in `cef_browser_shell.cpp`
  - [ ] Comment out `g_webview_hwnd` assignment
  - [ ] Add `// TODO: REPLACE WITH CefBrowserView` comments

- [ ] **Preserve working code as reference**
  - [ ] Add detailed comments explaining current functionality
  - [ ] Mark sections that will be replaced vs removed
  - [ ] Create reference map of current message handling

#### **Frontend Tasks:**
- [ ] **Comment out overlay routing**
  - [ ] Comment out `/settings`, `/wallet`, `/backup` routes
  - [ ] Add `// TODO: CONVERT TO PANELS` comments
  - [ ] Preserve existing component structure as reference

#### **Documentation Tasks:**
- [ ] **Create migration reference document**
  - [ ] Map current message flow patterns
  - [ ] Document current HWND relationships
  - [ ] List all overlay-specific functionality to migrate

---

### **Phase 1: Architecture Setup** ⏱️ *1-2 days*

#### **Frontend Tasks:**
- [ ] **Create new layout components**
  - [ ] `MainLayout.tsx` - overall browser layout
  - [ ] `HeaderArea.tsx` - toolbar and navigation
  - [ ] `WebViewArea.tsx` - web content display
  - [ ] `PanelArea.tsx` - sliding panel container
  - [ ] `PanelManager.tsx` - panel state management

- [ ] **Update main App.tsx**
  - [ ] Remove overlay routing (`/settings`, `/wallet`, `/backup`)
  - [ ] Add panel state management
  - [ ] Implement panel open/close logic
  - [ ] Add CSS transitions for slide-in/out

- [ ] **Create panel components**
  - [ ] `SettingsPanel.tsx` - convert from SettingsOverlayRoot
  - [ ] `WalletPanel.tsx` - convert from WalletOverlayRoot
  - [ ] `BackupPanel.tsx` - convert from BackupOverlayRoot

#### **C++ Backend Tasks:**
- [ ] **Comment out overlay creation functions**
  - [ ] `CreateSettingsOverlayWithSeparateProcess()`
  - [ ] `CreateWalletOverlayWithSeparateProcess()`
  - [ ] `CreateBackupOverlayWithSeparateProcess()`
  - [ ] Add `// TODO: REMOVE AFTER MIGRATION` comments

- [ ] **Update message handling**
  - [ ] Modify `OnProcessMessageReceived()` to handle panel state
  - [ ] Add panel open/close message types
  - [ ] Remove overlay-specific message handling

#### **Wallet Daemon Tasks:**
- [ ] **No changes required** ✅

---

### **Phase 2: Settings Panel Migration** ⏱️ *1 day*

#### **Frontend Tasks:**
- [ ] **Convert SettingsOverlayRoot to SettingsPanel**
  - [ ] Move all existing functionality
  - [ ] Update styling for panel layout
  - [ ] Remove overlay-specific code
  - [ ] Test open/close animations

- [ ] **Update header button handling**
  - [ ] Change settings button to toggle panel state
  - [ ] Remove overlay creation calls
  - [ ] Add panel state management

- [ ] **Test settings functionality**
  - [ ] Verify all existing features work
  - [ ] Test panel positioning and sizing
  - [ ] Verify keyboard input works

#### **C++ Backend Tasks:**
- [ ] **Update settings message handling**
  - [ ] Remove settings overlay creation
  - [ ] Add settings panel state messages
  - [ ] Test message flow

#### **Wallet Daemon Tasks:**
- [ ] **No changes required** ✅

---

### **Phase 3: Bridge & Hooks Migration** ⏱️ *2-3 days*

#### **Frontend Tasks:**
- [ ] **Update CEF messaging**
  - [ ] Move all `window.cefMessage.send()` calls to main process
  - [ ] Update message routing for single process
  - [ ] Test all existing API calls

- [ ] **Verify all hooks work**
  - [ ] `useBalance.ts` - test balance fetching
  - [ ] `useTransaction.ts` - test transaction flow
  - [ ] `useAddress.ts` - test address generation
  - [ ] `useBitcoinBrowser.ts` - test all wallet operations

- [ ] **Update bridge initialization**
  - [ ] Ensure `window.bitcoinBrowser` API works in main process
  - [ ] Test all bridge functions
  - [ ] Verify error handling

#### **C++ Backend Tasks:**
- [ ] **Consolidate message handling**
  - [ ] Move all overlay message handlers to main process
  - [ ] Update message routing logic
  - [ ] Test all message types work

- [ ] **Update WalletService calls**
  - [ ] Ensure all Go daemon calls work from main process
  - [ ] Test HTTP client functionality
  - [ ] Verify process management

#### **Wallet Daemon Tasks:**
- [ ] **No changes required** ✅

---

### **Phase 4: Wallet Panel Migration** ⏱️ *2-3 days*

#### **Frontend Tasks:**
- [ ] **Convert WalletOverlayRoot to WalletPanel**
  - [ ] Move all existing functionality
  - [ ] Update styling for panel layout
  - [ ] Test all wallet features

- [ ] **Update wallet button handling**
  - [ ] Change wallet button to toggle panel state
  - [ ] Remove overlay creation calls
  - [ ] Add panel state management

- [ ] **Test complete wallet functionality**
  - [ ] Address generation
  - [ ] Balance display
  - [ ] Transaction sending
  - [ ] All UI interactions

#### **C++ Backend Tasks:**
- [ ] **Update wallet message handling**
  - [ ] Remove wallet overlay creation
  - [ ] Add wallet panel state messages
  - [ ] Test all wallet operations

#### **Wallet Daemon Tasks:**
- [ ] **No changes required** ✅

---

### **Phase 5: Backup Panel Migration** ⏱️ *1 day*

#### **Frontend Tasks:**
- [ ] **Convert BackupOverlayRoot to BackupPanel**
  - [ ] Move all existing functionality
  - [ ] Update styling for panel layout
  - [ ] Test backup flow

- [ ] **Update backup button handling**
  - [ ] Change backup button to toggle panel state
  - [ ] Remove overlay creation calls
  - [ ] Add panel state management

#### **C++ Backend Tasks:**
- [ ] **Update backup message handling**
  - [ ] Remove backup overlay creation
  - [ ] Add backup panel state messages
  - [ ] Test backup functionality

#### **Wallet Daemon Tasks:**
- [ ] **No changes required** ✅

---

### **Phase 6: Cleanup & Testing** ⏱️ *1 day*

#### **Frontend Tasks:**
- [ ] **Remove commented overlay code**
  - [ ] Delete all `// TODO: REMOVE AFTER MIGRATION` sections
  - [ ] Remove unused overlay files
  - [ ] Clean up imports

- [ ] **Final testing**
  - [ ] Test all panels open/close correctly
  - [ ] Test all functionality works
  - [ ] Test keyboard input
  - [ ] Test panel positioning

#### **C++ Backend Tasks:**
- [ ] **Remove overlay creation functions**
  - [ ] Delete commented overlay functions
  - [ ] Remove overlay HWND management
  - [ ] Clean up unused includes

- [ ] **Final testing**
  - [ ] Test all message handling
  - [ ] Test Go daemon communication
  - [ ] Test window management

#### **Wallet Daemon Tasks:**
- [ ] **No changes required** ✅

---

## 🔧 **Technical Implementation Details**

### **Target Window Architecture:**
```
Main HWND (BitcoinBrowserWndClass)
├── header_hwnd (CEFHostWindow) ← React JSX renders here
│   ├── CefBrowserView ← Web content area (embedded as child)
│   └── PanelArea ← Sliding panels (settings, wallet, backup)
└── (other UI elements)
```

### **CefBrowserView Integration Requirements:**
- **Use CefWindowInfo::SetAsChild()** to embed in header_hwnd
- **NO separate CefWindow instances** - causes crashes
- **Proper parent-child relationship** with header_hwnd
- **Coordinate event handling** between React and CefBrowserView

### **New Panel Architecture:**
```typescript
// Panel state management
interface PanelState {
  settings: { open: boolean; data?: any };
  wallet: { open: boolean; data?: any };
  backup: { open: boolean; data?: any };
}

// Panel positioning - panels slide over webview area
const PANEL_WIDTH = 400;
const PANEL_HEIGHT = 600;
const PANEL_SLIDE_DURATION = 300; // ms
const WEBVIEW_Z_INDEX = 1;
const PANEL_Z_INDEX = 10; // Panels appear above webview
```

### **Message Flow Changes:**
```typescript
// Before: Multiple processes
window.cefMessage.send('overlay_show', ['settings']);

// After: Single process
setPanelState({ settings: { open: true } });
```

### **CSS Transitions & Z-Order Management:**
```css
.webview-area {
  position: relative;
  z-index: 1;
  width: 100%;
  height: 100%;
}

.panel-area {
  position: absolute;
  top: 0;
  right: 0;
  width: 400px;
  height: 100%;
  z-index: 10;
  transform: translateX(100%);
  transition: transform 300ms ease-in-out;
  background: white;
  box-shadow: -2px 0 10px rgba(0,0,0,0.1);
}

.panel-area.open {
  transform: translateX(0);
}
```

### **CefBrowserView Implementation Pattern:**
```cpp
// Correct approach - embed in existing HWND
CefWindowInfo window_info;
window_info.SetAsChild(header_hwnd, rect);
CefBrowserHost::CreateBrowser(window_info, handler, url, browser_settings, nullptr);

// WRONG - creates separate top-level window (causes crashes)
// CefWindow::CreateTopLevelWindow() - DO NOT USE
```

---

## ⚠️ **Risk Mitigation**

### **Critical Implementation Warnings:**
- **NEVER create separate CefWindow instances** - causes crashes
- **ALWAYS use CefWindowInfo::SetAsChild()** for embedding
- **Ensure parent window is visible** before creating browser
- **Coordinate event handling** between React and CefBrowserView
- **Manage z-order properly** for panels and webview

### **Rollback Strategy:**
- [ ] Keep all old code commented out until migration complete
- [ ] Use feature flags to switch between old/new implementations
- [ ] Test each phase thoroughly before proceeding
- [ ] Preserve working wrapper in cef-binaries (gitignored)

### **Testing Checklist:**
- [ ] All existing functionality preserved
- [ ] No performance regressions
- [ ] Keyboard input works correctly
- [ ] Panel animations smooth
- [ ] No memory leaks
- [ ] All error handling works
- [ ] **Single top-level window only**
- [ ] **CefBrowserView properly embedded in header_hwnd**
- [ ] **Panels slide over webview without conflicts**

---

## 📈 **Success Metrics**

### **Functionality:**
- [ ] All panels open/close correctly
- [ ] All wallet operations work
- [ ] All settings functionality preserved
- [ ] Keyboard input works in all panels

### **Performance:**
- [ ] Faster panel opening (no process creation)
- [ ] Lower memory usage (single process)
- [ ] Smoother animations
- [ ] Better responsiveness

### **Code Quality:**
- [ ] Simpler architecture
- [ ] Easier to maintain
- [ ] Follows modern browser patterns
- [ ] Better user experience

---

## 🎯 **Total Timeline: 7.5-10.5 days**

**Phase 0**: 0.5 days (Code Preservation & Reference)
**Phase 1**: 1-2 days (Architecture)
**Phase 2**: 1 day (Settings)
**Phase 3**: 2-3 days (Bridge & Hooks)
**Phase 4**: 2-3 days (Wallet)
**Phase 5**: 1 day (Backup)
**Phase 6**: 1 day (Cleanup)

---

## 📝 **Notes**

- **Wallet Daemon**: No changes required - all existing HTTP API endpoints work as-is
- **Microservices**: Yes, this is a valid microservices architecture with clear separation of concerns
- **Rollback**: Easy to rollback at any phase due to commented code preservation
- **Testing**: Each phase includes comprehensive testing to ensure no functionality loss
- **Wrapper Verified**: New CEF wrapper works correctly with old code - no compatibility issues
- **Architecture Key**: CefBrowserView must be embedded in header_hwnd, not separate window
- **Space Management**: Panels and webview share same space with proper z-order layering
