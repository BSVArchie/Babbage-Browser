# Babbage-Browser/BitcoinBrowser

A custom Web3 browser built on the Chromium Embedded Framework (CEF) with native integration for BitcoinSV smart contracts, secure wallets, and micropayments.

## 🔧 Project Structure


> Note: `cef-binaries/` is excluded from Git using `.gitignore`.

## 🚀 Goals

- ✅ CEF shell with secure wallet backend
- 🧱 Build the UI from scratch using React + Vite
- 🔐 Enforce native, secure signing (not in JavaScript)
- ⚙️ Smart contract integration with sCrypt (or custom) and Authrite
- 🎯 Support micropayments, token gating, and identity-bound access

## 📦 Tech Stack

| Layer | Technology |
|-------|------------|
| Browser Shell | C++ / Chromium Embedded Framework |
| UI | React + Vite (TypeScript) |
| Smart Contracts | sCrypt (BSV) |
| Native Wallet | C++ or Rust backend |
| Identity / Auth | Authrite Protocol (Babbage) |

## 🛠️ Setup (Coming Soon)

Instructions need be added for:

- Building the native CEF shell
- Running the React frontend in development
- Integrating `window.nativeWallet` bridge for UI ↔ native communication

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
- **Native control**: Full backend control over cookie, contract, and payment enforcement
- **Web3 reimagined**: Built for real micropayments, not fake dApps
- **Prioritize user experience**: Clean easy to use and understand

## 🧬 Babbage Protocol Compatibility

This project is being built to be compatible with apps written for Metanet Desktop and the Babbage Protocol ecosystem. The goal is to support seamless interaction with:

- **Authrite authentication requests**
- **Toolio-generated identities and WAB certificates**
- **BSV Wallet Adapter signing and address resolution**
- **MetanetDesktop-style storage and identity detection**: Identity and wallet information will be stored in AppData%/MetanetDesktop/identity.json
- **JSON structure compatible with Metanet's identity format**
- **Planning to inject a browser-side API for apps to access identity, e.g.**:
    window.identity = { ... }
    window.babbage.getPublicKey()
    window.babbage.signMessage(...)

---

This is an early-stage rewrite.
