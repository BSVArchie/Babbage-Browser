# Build Instructions - Babbage Browser

## 🎯 Overview

This document provides step-by-step instructions for building the Babbage Browser project. The build process involves multiple components: CEF binaries, C++ native shell, Python wallet backend, and React frontend.

## 📋 Prerequisites

### Required Software
- **Visual Studio 2022** (Community or Professional)
- **CMake** 3.20 or later
- **Python** 3.9 or later
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

### Step 2: Python Wallet Backend Setup

#### Install Python Dependencies
```bash
# Create virtual environment
python -m venv venv
venv\Scripts\activate  # Windows
# source venv/bin/activate  # Linux/Mac

# Install Bitcoin SV Python SDK
pip install bsv-sdk

# Install additional dependencies
pip install cryptography
pip install requests
pip install pydantic
```

#### Verify Python Installation
```bash
python -c "import bsv; print('bsv-sdk installed successfully')"
```

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

#### Start Python Wallet Daemon
```bash
# In separate terminal
cd python-wallet
python wallet_daemon.py
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

### Python Integration Issues
- [ ] **Process Communication**: Implement C++ ↔ Python communication
- [ ] **Error Handling**: Add proper error handling for Python calls
- [ ] **Security**: Implement secure process isolation for wallet daemon
- [ ] **Dependencies**: Verify all Python dependencies are available

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

### Python Environment
```bash
# requirements.txt
bsv-sdk>=1.0.0
cryptography>=41.0.0
requests>=2.31.0
pydantic>=2.0.0
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

#### Python Integration Issues
```bash
# Error: Python module not found
# Solution: Ensure Python virtual environment is activated
# Check: Verify all dependencies are installed
```

#### CMake Configuration Issues
```bash
# Error: CMake configuration failed
# Solution: Check all required libraries are installed via vcpkg
# Verify: OpenSSL, nlohmann/json, and other dependencies
```

---

*This build guide will be updated as the project evolves and build issues are resolved.*
