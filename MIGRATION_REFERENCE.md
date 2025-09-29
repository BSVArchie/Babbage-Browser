# Migration Reference Document

## Phase 0: Code Preservation & Reference Setup

### Current HWND Relationships (Before Migration)
```
Main HWND (BitcoinBrowserWndClass)
├── header_hwnd (CEFHostWindow) ← React content renders here
└── webview_hwnd (CEFHostWindow) ← Web browsing (TO BE REPLACED)
```

### Target HWND Relationships (After Migration)
```
Main HWND (BitcoinBrowserWndClass)
└── header_hwnd (CEFHostWindow) ← React content + CefBrowserView + Panels
    ├── CefBrowserView ← Web browsing (embedded as child)
    └── PanelArea ← Sliding panels (settings, wallet, backup)
```

### Current Message Flow Patterns
- `overlay_show_settings` → `CreateSettingsOverlayWithSeparateProcess()`
- `overlay_show_wallet` → `CreateWalletOverlayWithSeparateProcess()`
- `overlay_show_backup` → `CreateBackupOverlayWithSeparateProcess()`

### Overlay Functionality to Migrate
1. **Settings Overlay** (`SettingsOverlayRoot`)
   - Settings configuration UI
   - Wallet management options
   - System preferences

2. **Wallet Overlay** (`WalletOverlayRoot`)
   - Address generation
   - Balance display
   - Transaction sending
   - Wallet operations

3. **Backup Overlay** (`BackupOverlayRoot`)
   - Wallet backup process
   - Seed phrase display
   - Recovery options

### Webview Integration Points
- Currently: Separate `webview_hwnd` for web browsing
- Target: `CefBrowserView` embedded in `header_hwnd`
- Method: Use `CefWindowInfo::SetAsChild()` to embed

### Phase 0 Changes Made
- ✅ Commented out overlay creation function calls
- ✅ Commented out webview_hwnd creation
- ✅ Commented out overlay routing in React
- ✅ Added detailed TODO comments
- ✅ Preserved all code as reference

### Next Steps (Phase 1)
- Create new layout components
- Implement CefBrowserView integration
- Set up panel state management
- Add CSS transitions and z-order management
