# Systematic Migration Plan: Overlay Windows → Integrated Panels

## 🎯 **Migration Overview**

**Goal**: Convert from separate overlay HWNDs to integrated React panels within the main browser window, following modern browser architecture patterns.

**Current Architecture**: Separate CEF processes for each overlay (settings, wallet, backup)
**Target Architecture**: Single main process with React-managed panels

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

### **New Panel Architecture:**
```typescript
// Panel state management
interface PanelState {
  settings: { open: boolean; data?: any };
  wallet: { open: boolean; data?: any };
  backup: { open: boolean; data?: any };
}

// Panel positioning
const PANEL_WIDTH = 400;
const PANEL_HEIGHT = 600;
const PANEL_SLIDE_DURATION = 300; // ms
```

### **Message Flow Changes:**
```typescript
// Before: Multiple processes
window.cefMessage.send('overlay_show', ['settings']);

// After: Single process
setPanelState({ settings: { open: true } });
```

### **CSS Transitions:**
```css
.panel-area {
  transform: translateX(100%);
  transition: transform 300ms ease-in-out;
}

.panel-area.open {
  transform: translateX(0);
}
```

---

## ⚠️ **Risk Mitigation**

### **Rollback Strategy:**
- [ ] Keep all old code commented out until migration complete
- [ ] Use feature flags to switch between old/new implementations
- [ ] Test each phase thoroughly before proceeding

### **Testing Checklist:**
- [ ] All existing functionality preserved
- [ ] No performance regressions
- [ ] Keyboard input works correctly
- [ ] Panel animations smooth
- [ ] No memory leaks
- [ ] All error handling works

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

## 🎯 **Total Timeline: 7-10 days**

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
