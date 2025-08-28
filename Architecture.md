+----------------------------+
|        React UI Layer     |
|  - Panels / Pages / Hooks |
|  - TypeScript + Vite      |
+----------------------------+
            ↓
+----------------------------+
|   JS ↔ Native Bridge Layer |
|  - window.nativeWallet     |
|  - window.identity         |
+----------------------------+
            ↓
+----------------------------+
|     Native CEF Shell       |
|  - C++ / Chromium          |
|  - Wallet / Identity Core  |
|  - CEF Handlers            |
+----------------------------+
            ↓
+----------------------------+
|   Smart Contract Layer     |
|  - sCrypt / Authrite       |
|  - Token Gating / Access   |
+----------------------------+
            ↓
+----------------------------+
|     Wallet Backend Layer   |
|  - C++ or Rust             |
|  - Secure Signing / Keys   |
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
