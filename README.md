# Babbage-Browser

A custom Web3 browser built on the Chromium Embedded Framework (CEF) with native integration for BitcoinSV smart contracts, secure wallets, and micropayments.

## 🔧 Project Structure


> Note: `cef-binaries/` is excluded from Git using `.gitignore`.

## 🚀 Goals

- ✅ Minimal CEF shell with secure wallet backend
- 🧱 Rebuild the UI from scratch using React + Vite
- 🔐 Enforce native, secure signing (not in JavaScript)
- ⚙️ Smart contract integration with sCrypt and Authrite
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

Instructions will be added for:

- Building the native CEF shell
- Running the React frontend in development
- Integrating `window.nativeWallet` bridge for UI ↔ native communication

## 📁 Repository Notes

- This repo is being rebuilt from scratch.
- Original Metanet Desktop UI was removed in favor of a cleaner architecture.
- CEF binaries are local-only and not tracked by Git.

## 💡 Project Philosophy

- **Security-first**: Private keys and signing logic never exposed to JS
- **Native control**: Full backend control over cookie, contract, and payment enforcement
- **Web3 reimagined**: Built for real micropayments, not fake dApps

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

This is an early-stage rewrite. Expect rapid changes and API evolution.
