+----------------------------+
|        React UI Layer     |
|  - Panels / Pages / Hooks |
|  - TypeScript + Vite      |
|  🟡 Future: React Native   |
+----------------------------+
            ↓
+----------------------------+
|   JS ↔ Native Bridge Layer |
|  - window.bitcoinBrowser   |
|  - window.identity         |
+----------------------------+
            ↓
+----------------------------+
|     Native CEF Shell       |
|  - C++ / Chromium          |
|  - CEF Handlers            |
|  🟡 Future: Full Chromium  |
+----------------------------+
            ↓
+----------------------------+
|   Go Wallet Backend        |
|  - bitcoin-sv/go-sdk       |
|  - BEEF Transaction Support|
|  - SPV Verification        |
|  - Secure Key Management   |
|  🟡 Future: May migrate to |
|     Rust for max perf      |
+----------------------------+
            ↓
+----------------------------+
| Identity & Auth Layer      |
|  - BRC-100 Auth Framework  |
|  - BRC-52/103 Certificates |
|  - Type-42 Key Derivation  |
|  - Selective Disclosure    |
|  - SPV Identity Validation |
|  - BEEF Atomic Transactions|
+----------------------------+
            ↓
+----------------------------+
| Bitcoin SV Blockchain      |
|  - TAAL, GorillaPool       |
|  - Terranode, ARC Formats  |
|  🟡 Multi-platform builds  |
+----------------------------+



Communication Architecture
┌─────────────────┐    V8 Injection    ┌─────────────────┐
│   C++ Backend   │ ──────────────────→ │  Render Process │
│                 │                     │                 │
│ • Identity      │                     │ • window.bitcoinBrowser.identity.get() │
│ • Navigation    │                     │ • window.bitcoinBrowser.navigation.navigate() │
│ • Panel Control │                     │ • window.bitcoinBrowser.overlayPanel.open() │
└─────────────────┘                     └─────────────────┘
         │                                        │
         │ Process Messages                       │ JavaScript Execution
         │                                        │
         ▼                                        ▼
┌─────────────────┐                     ┌─────────────────┐
│ Browser Process │                     │   React App     │
│ Message Handler │                     │                 │
│                 │                     │ • Panel triggers │
│ • navigate      │                     │ • State updates  │
│ • overlay_open  │                     │ • UI rendering   │
└─────────────────┘                     └─────────────────┘



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

    B4 --> B4a["Overlay Window Management"]
    B4 --> B4b["Custom Render Handler (MyOverlayRenderHandler)"]
    B4 --> B4c["Panel Triggering (window.triggerPanel)"]

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
    C2 --> C2c["Overlay Root (/overlay) - Overlay panel system"]

    C3 --> C3a["Panels"]
    C3 --> C3b["Main Browser Interface"]

    C3a --> C3a1["Wallet Panel Layout (WalletPanelLayout.tsx)"]
    C3a --> C3a2["Wallet Panel Content (WalletPanelContent.tsx)"]
    C3a --> C3a3["Settings Panel Layout (SettingsPanelLayout.tsx)"]
    C3a --> C3a4["Backup Modal (BackupModal.tsx)"]

    C3b --> C3b1["Navigation Toolbar"]
    C3b --> C3b2["Address Bar"]
    C3b --> C3b3["Wallet Button"]
    C3b --> C3b4["Settings Button"]

    C4 --> C4a["useBitcoinBrowser - Native API integration"]

    C5 --> C5a["bitcoinBrowser.d.ts - Native API types"]
    C5 --> C5b["identity.d.ts - Identity data types"]

    C6 --> C6a["initWindowBridge.ts - Native communication setup"]

    C7 --> C7a["Vite Configuration (vite.config.ts)"]
    C7 --> C7b["TypeScript Config (tsconfig.json)"]
    C7 --> C7c["ESLint Configuration (eslint.config.js)"]

    D --> D1["Main Configuration (CMakeLists.txt)"]
    D --> D2["Source Compilation"]
    D --> D3["Dependencies"]
    D --> D4["CEF Integration"]
    D --> D5["Build Outputs"]
    D --> D6["Module Subdirectories"]

    D2 --> D2a["C++ Source Files"]
    D2 --> D2b["Header Files"]
    D2 --> D2c["Test Files"]

    D3 --> D3a["OpenSSL (via vcpkg)"]
    D3 --> D3b["nlohmann/json (via vcpkg)"]
    D3 --> D3c["Windows System Libraries"]

    D4 --> D4a["Binary Distribution Paths"]
    D4 --> D4b["Wrapper Library Linking"]
    D4 --> D4c["Runtime File Copying"]

    D5 --> D5a["BitcoinBrowserShell.exe"]
    D5 --> D5b["CEF Runtime Files"]
    D5 --> D5c["Debug/Release Artifacts"]

    D6 --> D6a["src/core/ (CMakeLists.txt)"]
    D6 --> D6b["tests/ (CMakeLists.txt)"]

    E --> E1["CEF (Chromium Embedded Framework)"]
    E --> E2["Cryptography"]
    E --> E3["Windows System"]
    E --> E4["Package Management"]

    E1 --> E1a["Binary Distribution"]
    E1 --> E1b["Header Files"]
    E1 --> E1c["Wrapper Library (libcef_dll_wrapper)"]
    E1 --> E1d["Runtime DLLs (libcef.dll, cef_sandbox.lib)"]
    E1 --> E1e["Resources & Locales"]

    E2 --> E2a["OpenSSL (AES, EC, SHA, RIPEMD)"]
    E2 --> E2b["nlohmann/json (JSON parsing)"]

    E3 --> E3a["user32, gdi32, ole32, oleaut32"]
    E3 --> E3b["comdlg32, shlwapi, uuid, winmm"]
    E3 --> E3c["dbghelp, delayimp, shell32"]
    E3 --> E3d["advapi32, dwmapi, version"]

    E4 --> E4a["vcpkg (C++ dependency manager)"]

    F --> F1["README.md - Project overview and setup"]
    F --> F2["Architecture.md - System architecture diagram"]
    F --> F3["BRC-100 - Protocol compatibility documentation"]

    G --> G1[".gitignore - Version control exclusions"]
    G --> G2[".vscode/ - VS Code workspace settings"]
    G --> G3["Build Artifacts"]

    G2 --> G2a["settings.json - Editor configuration"]
    G2 --> G2b["c_cpp_properties.json - C++ IntelliSense"]

    G3 --> G3a["cef-native/build/ - CMake build output"]
    G3 --> G3b["cef-binaries/ - CEF distribution (gitignored)"]
    G3 --> G3c["frontend/node_modules/ - npm dependencies (gitignored)"]
