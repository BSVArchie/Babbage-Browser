# Build Instructions - Bitcoin Browser

## 🎯 Overview

This document provides step-by-step instructions for building the Bitcoin Browser project. The build process involves multiple components: CEF binaries, C++ native shell, Go wallet backend, and React frontend.

## 📋 Prerequisites

### Required Software
- **Visual Studio 2022** (Community or Professional)
- **CMake** 3.20 or later
- **Go** 1.21 or later
- **Node.js** 18 or later
- **Git** for version control

### Required Libraries
- **vcpkg** (C++ package manager)
- **OpenSSL** (via vcpkg)
- **nlohmann/json** (via vcpkg)

## 🔧 Build Process

### Step 1: CEF Binaries Setup

#### Download CEF Binaries
```bash
# Note: CEF binaries are gitignored and need to be downloaded separately
# Download CEF binaries for Windows x64
# Version: [TO BE DETERMINED - check cef-binaries directory]
# Place in: ./cef-binaries/
```

#### Build CEF Wrapper
```bash
cd cef-binaries/libcef_dll/wrapper
mkdir build
cd build

# Configure CMake with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake

# Build the wrapper library
cmake --build . --config Release
```

**Note**: The wrapper CMakeLists.txt needs to be copied from the local repository. The exact CEF version and paths are currently unknown and need to be determined.

### Step 2: Wallet Backend Setup

**⚠️ TWO WALLET IMPLEMENTATIONS:** Choose ONE to run (both use port 3301):

#### Option A: Go Wallet (BSV SDK Implementation)

```bash
# Navigate to Go wallet directory
cd go-wallet

# Dependencies already configured in go.mod
go mod download

# Build the wallet executable
go build -o bitcoin-wallet.exe

# Run the wallet
./bitcoin-wallet.exe
# Or use the batch file: ./start-wallet.bat
# Server starts on http://127.0.0.1:3301
```

**Features:**
- Official BSV Go SDK (`v1.2.9`)
- HD wallet with BIP44 derivation
- Transaction creation, signing, broadcasting
- Full BRC-100 authentication support
- CEF browser integration tested

#### Option B: Rust Wallet (Custom Implementation)

```bash
# Navigate to Rust wallet directory
cd rust-wallet

# Build the wallet executable
cargo build --release

# Run the wallet server
cargo run --release
# Or: ./target/release/bitcoin-browser-wallet.exe
# Server starts on http://127.0.0.1:3301
```

**Features:**
- Custom Actix-web HTTP server
- BRC-103/104 mutual authentication
- Custom BSV ForkID SIGHASH implementation
- Transaction creation, signing, broadcasting
- Confirmed mainnet transactions

**🔧 IMPORTANT NOTES:**
- **Both use port 3301** - Only run ONE at a time!
- **Shared wallet.json** - Located at `%APPDATA%/BabbageBrowser/wallet/wallet.json`
- **Choose your implementation** - Testing both for production decision
- **Stop one before starting the other** - Port conflict if both run simultaneously

#### Test the APIs

**Both wallets use Port 3301:**

**Go Wallet Endpoints:**
- `GET http://localhost:3301/health` - Health check
- `GET http://localhost:3301/wallet/info` - Get wallet information
- `GET http://localhost:3301/wallet/balance` - Get total balance
- `POST http://localhost:3301/transaction/send` - Send transaction
- `GET http://localhost:3301/brc100/status` - BRC-100 service status

**Rust Wallet Endpoints:**
- `GET http://localhost:3301/wallet/status` - Wallet status
- `POST http://localhost:3301/getVersion` - Get wallet version
- `POST http://localhost:3301/getPublicKey` - Get public key
- `POST http://localhost:3301/createHmac` - Create HMAC for authentication
- `POST http://localhost:3301/verifyHmac` - Verify HMAC
- `POST http://localhost:3301/createSignature` - Create message signature
- `POST http://localhost:3301/verifySignature` - Verify message signature
- `POST http://localhost:3301/.well-known/auth` - BRC-104 authentication
- `POST http://localhost:3301/createAction` - Create transaction
- `POST http://localhost:3301/signAction` - Sign transaction
- `POST http://localhost:3301/processAction` - Process and broadcast transaction

**Test with PowerShell:**
```powershell
# Test Go wallet (make sure Go wallet is running)
Invoke-RestMethod -Uri "http://localhost:3301/health" -Method GET

# Test Rust wallet (make sure Rust wallet is running, stop Go wallet first)
Invoke-RestMethod -Uri "http://localhost:3301/wallet/status" -Method GET
```

#### Important Notes
- **Single port**: Both wallets use port 3301 - only ONE can run at a time
- **Shared storage**: Both use `%APPDATA%/BabbageBrowser/wallet/wallet.json`
- **Choose implementation**: Run either Go OR Rust wallet, not both
- **Production blockchain**: Both wallets work with real BSV network
- **Confirmed transactions**: Rust wallet has multiple successful mainnet broadcasts
- **Development/testing**: Comparing both implementations for production decision

### Step 3: React Frontend Setup

#### Install Node.js Dependencies
```bash
cd frontend
npm install
```

#### Start Development Server
```bash
npm run dev
# Frontend will be available at http://127.0.0.1:5137
```

### Step 4: C++ Native Shell Build

#### Configure CMake
```bash
cd cef-native
mkdir build
cd build

# Configure with vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake

# Note: CEF paths are hardcoded and need to be updated
# Current issues:
# - CEF binary paths need to be determined
# - vcpkg toolchain path needs to be specified
# - OpenSSL and nlohmann/json paths are hardcoded
```

#### Build Native Shell
```bash
# Build Release configuration
cmake --build . --config Release

# Build Debug configuration (for development)
cmake --build . --config Debug
```

### Step 5: Integration Testing

#### Start Go Wallet Daemon
```bash
# In separate terminal
cd go-wallet
go run main.go
```

#### Run Native Shell
```bash
# From cef-native/build/Release/
./BitcoinBrowserShell.exe
```

## 🚨 Known Issues & TODOs

### CEF Integration Issues
- [ ] **CEF Version**: Determine exact CEF version and download links
- [ ] **Hardcoded Paths**: Update CMakeLists.txt with correct CEF paths
- [ ] **vcpkg Toolchain**: Specify correct vcpkg toolchain file path
- [ ] **OpenSSL Paths**: Fix hardcoded OpenSSL library paths
- [ ] **Wrapper Build**: Ensure wrapper library builds correctly

### Go Integration Issues
- [ ] **Process Communication**: Implement C++ ↔ Go communication
- [ ] **Error Handling**: Add proper error handling for Go calls
- [ ] **Security**: Implement secure process isolation for wallet daemon
- [ ] **Dependencies**: Verify all Go dependencies are available

### Build System Issues
- [ ] **Cross-Platform**: Test build process on different platforms
- [ ] **CI/CD**: Set up automated build pipeline
- [ ] **Dependencies**: Automate dependency installation
- [ ] **Version Management**: Implement proper versioning for all components

## 🔧 Development Environment Setup

### Visual Studio Configuration
```json
// .vscode/c_cpp_properties.json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/cef-binaries/include",
                "${workspaceFolder}/cef-native/include",
                "${vcpkgRoot}/installed/x64-windows/include"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.22000.0",
            "compilerPath": "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.37.32822/bin/Hostx64/x64/cl.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-msvc-x64"
        }
    ]
}
```

### Go Environment
```go
// go.mod
module browser-wallet

go 1.21

require (
    github.com/bsv-blockchain/go-sdk v1.2.9
    github.com/gorilla/websocket v1.5.0
    github.com/sirupsen/logrus v1.9.0
)
```

## 🚀 Future Build Considerations

### Multi-Platform Support
- 🟡 **Windows**: Current CEF implementation
- 🟡 **macOS**: CEF with Cocoa integration
- 🟡 **Linux**: CEF with GTK integration
- 🟡 **Mobile**: React Native with native modules

### Build Optimizations
- 🟡 **Incremental Builds**: Optimize CMake for faster rebuilds
- 🟡 **Parallel Compilation**: Use multiple cores for faster builds
- 🟡 **Dependency Management**: Automate vcpkg package installation
- 🟡 **Cross-Compilation**: Support building for different architectures

### CI/CD Pipeline
- 🟡 **GitHub Actions**: Automated builds on multiple platforms
- 🟡 **Docker**: Containerized build environment
- 🟡 **Artifact Management**: Automated release packaging
- 🟡 **Testing**: Automated integration testing

## 📝 Build Troubleshooting

### Common Issues

#### CEF Binary Issues
```bash
# Error: CEF binaries not found
# Solution: Download and extract CEF binaries to cef-binaries/
# Check: Verify CEF version matches wrapper requirements
```

#### vcpkg Issues
```bash
# Error: vcpkg toolchain not found
# Solution: Install vcpkg and specify correct toolchain path
# Example: -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

#### Go Integration Issues
```bash
# Error: Go module not found
# Solution: Ensure Go module is initialized and dependencies are downloaded
# Check: Run 'go mod tidy' to verify all dependencies
```

#### CMake Configuration Issues
```bash
# Error: CMake configuration failed
# Solution: Check all required libraries are installed via vcpkg
# Verify: OpenSSL, nlohmann/json, and other dependencies
```

---

*This build guide will be updated as the project evolves and build issues are resolved.*
