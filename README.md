# Babbage-Browser/BitcoinBrowser

A custom Web3 browser built on the Chromium Embedded Framework (CEF) with native BitcoinSV wallet for secure authentication, micropayments, and Electronic Data Interchange (EDI- smart contracts).

## 🔧 Project Structure

> Note: `cef-binaries/` is excluded from Git using `.gitignore`.

## 🚀 Goals

- ✅ CEF shell with secure wallet backend
- 🧱 Build the UI from scratch using React + Vite
- 🔐 Enforce native, secure signing (not in JavaScript)
- ⚙️ Smart contract integration with sCrypt (or custom) and BRC-100/Authrite
- 🎯 Support micropayments, token gating, and identity-bound access

## 📦 Tech Stack

| Layer | Technology | Notes |
|-------|------------|-------|
| Browser Shell | C++ / Chromium Embedded Framework | 🟡 *Future: Consider full Chromium build* |
| UI | React + Vite (TypeScript) | 🟡 *Future: Consider React Native for mobile* |
| Native Wallet | **Python** (bsv-sdk) | 🟡 *PoC: Will migrate to Rust for production* |
| Key Derivation | **PBKDF2** (temporary) | 🟡 *Future: Upgrade to Argon2 for production* |
| Identity / Auth | BRC-100 (Authrite Protocol (Babbage)) | |
| Smart Contracts | sCrypt (BSV) | |
| Blockchain Integration | Bitcoin SV (TAAL, GorillaPool) | 🟡 *Multi-platform: Windows, Mac, Mobile* |

## 🛠️ Setup (Coming Soon)

Instructions need be added for:

- Building the native CEF shell
- Setting up Python wallet backend (bsv-sdk)
- Running the React frontend in development
- Integrating `window.bitcoinBrowser` bridge for UI ↔ native communication
- CEF binaries download and wrapper compilation

## 📁 Repository Notes

- CEF binaries are local-only and not tracked by Git.
- The cef-native and cef-binaries/libcef_dll/wrapper layers are independently compiled but logically connected:
    - The wrapper is built as a standalone static library (libcef_dll_wrapper.lib)
    - Your native shell links to that static lib manually

    BABBAGE-BROWSER (BitcoinBrowser)/
    ├── .vscode/                     → VSCode workspace configs
    │
    ├── cef-binaries/               → CEF binaries and libcef_dll wrapper source (not tracked by Git)
    │   └── libcef_dll/
    │       └── wrapper/            → Custom-built wrapper compiled to static lib (needs the CMakeList.txt)
    │
    ├── cef-native/                 → Native C++ shell for browser logic
    │   ├── build/                  → Local CMake/MSVC build artifacts
    │   ├── include/
    │   │   ├── core/               → Wallet, identity, and navigation headers
    │   │   └── handlers/           → CEF event hook headers (client, render, etc.)
    │   ├── src/
    │   │   ├── core/               → Backend implementations for wallet and identity
    │   │   └── handlers/           → CEF app/client/render lifecycle implementations
    │   └── tests/                  → Native shell test harness and main entrypoint
    │
    ├── frontend/                   → React + Vite UI
    │   ├── public/                 → Static assets served by Vite
    │   ├── src/
    │   │   ├── components/panels/  → Wallet UI, tabs, settings panels
    │   │   ├── hooks/              → Shared logic (e.g. `useBitcoinBrowser`)
    │   │   ├── pages/              → Page-level views like Browser and Welcome screens
    │   │   └── types/              → TypeScript types (identity, API contracts)
    │   ├── index.html              → App entrypoint (served by Vite)
    │   └── main.tsx                → React bootstrap
    │
    ├── .gitignore
    ├── README.md
    ├── vite.config.ts             → Vite config (frontend build + dev server)
    ├── tsconfig*.json             → TypeScript configurations
    ├── package.json               → Frontend dependencies and scripts
    └── eslint.config.js           → Linting setup

## 💡 Project Philosophy

- **Security-first**: Private keys and signing logic never exposed to JS
- **Native control**: Full backend control over cookie, adds, contract, InterPlanetary File System, and payment enforcement
- **Web3 reimagined**: Built for real micropayments, not fake dApps
- **Prioritize user experience**: Clean easy to use and understand

## 🔒 Security Architecture

### Why Native Wallet Backend?

**JavaScript Security Vulnerabilities:**
- **Process Isolation**: JavaScript runs in the browser's render process, which is inherently less secure than native processes
- **XSS Attack Surface**: Malicious websites could potentially access wallet functions through cross-site scripting attacks
- **Extension Interference**: Browser extensions or injected scripts could intercept wallet operations
- **Memory Exposure**: Private keys stored in JavaScript variables are accessible through console inspection, memory dumps, and developer tools

**Native Backend Benefits:**
- **Process Separation**: Wallet operations happen in isolated Python daemon processes, completely separate from web content
- **Memory Protection**: Python daemon provides stronger memory protection than JavaScript
- **Cryptographic Libraries**: Direct access to Bitcoin SV Python SDK (bsv-sdk) with BEEF and SPV support
- **Attack Surface Reduction**: Even if a website compromises the render process, it cannot access the wallet backend
| 🟡 *PoC: Will migrate to Rust for production* |

**Architecture Security:**
- **Controlled Bridge API**: Only safe, high-level functions are exposed through `window.bitcoinBrowser`
- **Multi-Process CEF**: Leverages Chromium's natural security boundaries between processes
- **Python Daemon Isolation**: Private keys never leave the isolated Python wallet process
- **Real Financial Security**: Built for production use where real money is at stake, not just development/testing

## 🧬 BRC-100 Protocol Compatibility

This project is being built to support apps that follow the **BRC-100 authentication and identity standards**, enabling secure, privacy-preserving interaction between wallets and applications. The goal is to ensure seamless compatibility with:

- **Toolio-generated identities and WAB certificates**
- **MetanetDesktop-style storage and identity detection**: Identity and wallet information will be stored in AppData%/MetanetDesktop/identity.json
- **BRC-52/103 identity certificates** with selective disclosure
- **Type-42 key derivation** for encrypted P2P channels
- **BEEF-formatted atomic transactions** for identity-bound actions
- **SPV-based identity and transaction verification**
- **Browser-side API injection for identity access**, e.g.:
  ```js
  window.bitcoinBrowser.identity.get()
  window.bitcoinBrowser.brc100.getPublicKey()
  window.bitcoinBrowser.brc100.signMessage(...)
  window.bitcoinBrowser.brc100.getCertificate()

---

This is an early-stage rewrite.
