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

### Step 2: Go Wallet Backend Setup

#### Install Go Dependencies
```bash
# Navigate to wallet directory
cd go-wallet

# Initialize Go module
go mod init browser-wallet

# Install Bitcoin SV Go SDK (correct module path)
go get github.com/bsv-blockchain/go-sdk

# Install additional dependencies
go get github.com/gorilla/mux
go get github.com/gorilla/websocket
go get github.com/sirupsen/logrus

# Resolve all dependencies
go mod tidy
```

#### Build and Run Wallet Daemon
```bash
# Build the wallet daemon
go build -o wallet.exe main.go

# Run the wallet daemon
./wallet.exe
```

#### Test the API
The wallet daemon provides these endpoints:
- `GET http://localhost:8080/health` - Health check
- `GET http://localhost:8080/identity/get` - Get wallet identity
- `POST http://localhost:8080/identity/markBackedUp` - Mark wallet as backed up

**Test with PowerShell** (in a separate terminal while server is running):
```powershell
# Health check
Invoke-RestMethod -Uri "http://localhost:8080/health" -Method GET

# Get identity
Invoke-RestMethod -Uri "http://localhost:8080/identity/get" -Method GET

# Mark as backed up
Invoke-RestMethod -Uri "http://localhost:8080/identity/markBackedUp" -Method POST
```

#### Important Notes
- The wallet daemon must be running for any wallet functionality
- Identity files are stored in `%APPDATA%/BabbageBrowser/identity.json`
- The daemon runs on port 8080 by default
- Private keys are currently stored in plain text (will be encrypted in production)

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
module babbage-browser-wallet

go 1.21

require (
    github.com/bitcoin-sv/go-sdk v1.0.0
    github.com/gorilla/mux v1.8.0
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
