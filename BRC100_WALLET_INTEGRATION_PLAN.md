# BRC-100 Wallet Integration Implementation Plan

## Overview

This document outlines the step-by-step plan for implementing BRC-100 wallet integration in the Bitcoin Browser. The implementation will add HTTP endpoints to the Go daemon to enable communication with BRC-100 compatible websites.

## Research Findings

### Protocol Analysis
- **Communication Method**: HTTP requests (not WebSocket)
- **Standard Ports**: 3301 (primary), 3321 (secondary)
- **Discovery Endpoint**: `/getVersion`
- **Message Format**: JSON over HTTP

### Site Compatibility
- [peerpay.babbage.systems](https://peerpay.babbage.systems/) - Uses ports 3301/3321
- [paymail.us](https://paymail.us/) - Uses ports 3301/3321/2121
- [thryll.online](https://www.thryll.online/) - Uses ports 3301/3321

### Expected Response Formats
```json
// /getVersion response
{
  "version": "BitcoinBrowserWallet v0.0.1",
  "capabilities": ["getVersion", "getPublicKey", "createAction", "signAction", "processAction"],
  "brc100": true,
  "timestamp": "2024-01-01T00:00:00Z"
}

// /getPublicKey response
{
  "publicKey": "02a1b2c3d4e5f6...",
  "address": "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa",
  "index": 0
}
```

## Implementation Plan

### Phase 1: Port Configuration and Basic Endpoints

#### Step 1: Change Go Daemon Port
**File**: `go-wallet/main.go`
**Location**: Line 726

```go
// Change from:
log.Fatal(http.ListenAndServe(":8080", nil))

// To:
log.Fatal(http.ListenAndServe(":3301", nil))
```

**Update HTTP Interceptor**:
**File**: `cef-native/src/handlers/simple_handler.cpp`
**Location**: Line 1398

```cpp
// Change from:
if (url.find("localhost:8080") != std::string::npos) {

// To:
if (url.find("localhost:3301") != std::string::npos) {
```

#### Step 2: Add BRC-100 Endpoints
**File**: `go-wallet/main.go`
**Location**: Add before server starts (around line 680)

```go
// Add BRC-100 wallet endpoints
http.HandleFunc("/getVersion", handleGetVersion)
http.HandleFunc("/getPublicKey", handleGetPublicKey)
http.HandleFunc("/createAction", handleCreateAction)
http.HandleFunc("/signAction", handleSignAction)
http.HandleFunc("/processAction", handleProcessAction)
```

#### Step 3: Implement Endpoint Handlers
**File**: `go-wallet/main.go`
**Location**: Add after existing handlers

```go
// handleGetVersion returns wallet version and capabilities
func handleGetVersion(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "GET" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    response := map[string]interface{}{
        "version": "BitcoinBrowserWallet v0.0.1",
        "capabilities": []string{
            "getVersion",
            "getPublicKey",
            "createAction",
            "signAction",
            "processAction",
        },
        "brc100": true,
        "timestamp": time.Now(),
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// handleGetPublicKey returns the wallet's current public key
func handleGetPublicKey(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "GET" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    // Get current wallet address and public key
    addressInfo, err := walletService.walletManager.GetCurrentAddress()
    if err != nil {
        http.Error(w, fmt.Sprintf("Failed to get wallet address: %v", err), http.StatusInternalServerError)
        return
    }

    response := map[string]interface{}{
        "publicKey": addressInfo.PublicKey,
        "address":   addressInfo.Address,
        "index":     addressInfo.Index,
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// handleCreateAction creates a new BRC-100 action
func handleCreateAction(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "POST" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    // For now, return a placeholder response
    response := map[string]interface{}{
        "success": true,
        "message": "BRC-100 action creation not yet implemented",
        "actionId": "placeholder",
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// handleSignAction signs a BRC-100 action
func handleSignAction(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "POST" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    // For now, return a placeholder response
    response := map[string]interface{}{
        "success": true,
        "message": "BRC-100 action signing not yet implemented",
        "signature": "placeholder",
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// handleProcessAction processes a completed BRC-100 action
func handleProcessAction(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "POST" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    // For now, return a placeholder response
    response := map[string]interface{}{
        "success": true,
        "message": "BRC-100 action processing not yet implemented",
        "result": "placeholder",
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}
```

### Phase 2: Domain Whitelist Integration

#### Step 4: Add Domain Checking to BRC-100 Endpoints
**File**: `go-wallet/main.go`
**Location**: Modify existing handlers

```go
// Add domain checking helper function
func checkDomainWhitelist(w http.ResponseWriter, r *http.Request) bool {
    // Extract domain from Origin header
    origin := r.Header.Get("Origin")
    if origin == "" {
        return true // Allow requests without Origin header (local testing)
    }

    // Parse domain from origin (e.g., "https://peerpay.babbage.systems" -> "peerpay.babbage.systems")
    domain := extractDomainFromOrigin(origin)

    // Check if domain is whitelisted
    if !domainWhitelistManager.IsDomainWhitelisted(domain) {
        response := map[string]interface{}{
            "error": "Domain not whitelisted",
            "domain": domain,
            "requiresAuth": true,
        }
        w.Header().Set("Content-Type", "application/json")
        w.WriteHeader(http.StatusForbidden)
        json.NewEncoder(w).Encode(response)
        return false
    }

    return true
}

func extractDomainFromOrigin(origin string) string {
    // Remove protocol (http:// or https://)
    if strings.HasPrefix(origin, "http://") {
        return strings.TrimPrefix(origin, "http://")
    }
    if strings.HasPrefix(origin, "https://") {
        return strings.TrimPrefix(origin, "https://")
    }
    return origin
}

// Update handleGetVersion to check domain whitelist
func handleGetVersion(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "GET" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    // Check domain whitelist
    if !checkDomainWhitelist(w, r) {
        return
    }

    // Continue with normal response...
    response := map[string]interface{}{
        "version": "BitcoinBrowserWallet v0.0.1",
        "capabilities": []string{
            "getVersion",
            "getPublicKey",
            "createAction",
            "signAction",
            "processAction",
        },
        "brc100": true,
        "timestamp": time.Now(),
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// Update handleGetPublicKey to check domain whitelist
func handleGetPublicKey(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "GET" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    // Check domain whitelist
    if !checkDomainWhitelist(w, r) {
        return
    }

    // Continue with normal response...
    addressInfo, err := walletService.walletManager.GetCurrentAddress()
    if err != nil {
        http.Error(w, fmt.Sprintf("Failed to get wallet address: %v", err), http.StatusInternalServerError)
        return
    }

    response := map[string]interface{}{
        "publicKey": addressInfo.PublicKey,
        "address":   addressInfo.Address,
        "index":     addressInfo.Index,
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}
```

#### Step 5: Add Domain Whitelist Endpoint for BRC-100
**File**: `go-wallet/main.go`
**Location**: Add with other BRC-100 endpoints

```go
// Add domain whitelist endpoint for BRC-100 requests
http.HandleFunc("/domain/whitelist/add", func(w http.ResponseWriter, r *http.Request) {
    enableCORS(w, r)
    if r.Method != "POST" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    var req struct {
        Domain      string `json:"domain"`
        IsPermanent bool   `json:"isPermanent"`
    }

    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, "Invalid request body", http.StatusBadRequest)
        return
    }

    if err := domainWhitelistManager.AddToWhitelist(req.Domain, req.IsPermanent); err != nil {
        http.Error(w, fmt.Sprintf("Failed to add domain: %v", err), http.StatusInternalServerError)
        return
    }

    response := map[string]interface{}{
        "success": true,
        "message": "Domain added to whitelist",
        "domain":  req.Domain,
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
})
```

### Phase 3: HTTP Interceptor Updates

#### Step 6: Update HTTP Interceptor for BRC-100
**File**: `cef-native/src/core/HttpRequestInterceptor.cpp`
**Location**: Add BRC-100 detection logic

```cpp
// Add BRC-100 wallet detection
bool isBRC100WalletRequest(const std::string& url) {
    return url.find("localhost:3301") != std::string::npos ||
           url.find("localhost:3321") != std::string::npos;
}

// In OnBeforeRequest method, add:
if (isBRC100WalletRequest(url)) {
    // Extract domain from referrer
    std::string domain = extractDomainFromReferrer(referrer);

    // Check if domain is whitelisted
    if (!isDomainWhitelisted(domain)) {
        // Trigger BRC-100 auth modal
        triggerBRC100AuthModal(domain, url, method, postData);
        return true; // Block request until user responds
    }
}
```

### Phase 4: Testing and Validation

#### Step 7: Test Basic Connectivity
1. **Start Go daemon**:
   ```bash
   cd go-wallet
   go run main.go
   ```

2. **Test `/getVersion` endpoint**:
   ```bash
   curl -X GET http://localhost:3301/getVersion
   ```

3. **Expected response**:
   ```json
   {
     "version": "BitcoinBrowserWallet v0.0.1",
     "capabilities": ["getVersion", "getPublicKey", "createAction", "signAction", "processAction"],
     "brc100": true,
     "timestamp": "2024-01-01T00:00:00Z"
   }
   ```

#### Step 8: Test with BRC-100 Sites
1. **Start browser** and navigate to:
   - [peerpay.babbage.systems](https://peerpay.babbage.systems/)
   - [paymail.us](https://paymail.us/)
   - [thryll.online](https://www.thryll.online/)

2. **Check console logs** for:
   - ✅ Successful `/getVersion` calls
   - ❌ No more "ERR_CONNECTION_REFUSED" errors
   - ❌ No more "No wallet available" messages

### Phase 5: BRC-100 Authentication Flow

#### Step 9: Fix BRC-100 Authentication Endpoints
**File**: `go-wallet/brc100_api.go`
**Location**: Update challenge handler

```go
// Update handleAuthChallenge to sign site's challenge instead of creating one
func handleAuthChallenge(w http.ResponseWriter, r *http.Request, service *BRC100Service) {
    enableCORS(w, r)
    if r.Method != "POST" {
        http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
        return
    }

    var req struct {
        Challenge string `json:"challenge"` // Site provides this
        AppID     string `json:"appId"`
    }

    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, fmt.Sprintf("Invalid request body: %v", err), http.StatusBadRequest)
        return
    }

    // Sign the site's challenge with wallet's private key
    signature, err := service.signWithWalletPrivateKey([]byte(req.Challenge))
    if err != nil {
        response := BRC100Response{
            Success: false,
            Error:   fmt.Sprintf("Failed to sign challenge: %v", err),
        }
        w.Header().Set("Content-Type", "application/json")
        json.NewEncoder(w).Encode(response)
        return
    }

    response := BRC100Response{
        Success: true,
        Data: map[string]interface{}{
            "challenge": req.Challenge,
            "signature": signature,
            "publicKey": service.getCurrentWalletPublicKey(),
        },
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// Add helper function to get current wallet public key
func (service *BRC100Service) getCurrentWalletPublicKey() string {
    addressInfo, err := service.walletManager.GetCurrentAddress()
    if err != nil {
        return ""
    }
    return addressInfo.PublicKey
}
```

## HD Wallet Public Key Analysis

### Current HD Wallet Structure
Our HD wallet already stores public keys with each address:

```go
type AddressInfo struct {
    Index     int    `json:"index"`
    Address   string `json:"address"`
    PublicKey string `json:"publicKey"`  // ✅ Already stored!
    Used      bool   `json:"used"`
    Balance   int64  `json:"balance"`
}
```

### Public Key Storage and Usage

#### What BRC-100 Sites Expect:
- **Public Key**: Compressed public key in hex format
- **Address**: Bitcoin address associated with the public key
- **Index**: Address index for reference

#### What We Already Have:
- ✅ **Public Key Storage**: Each address stores its public key in hex format
- ✅ **Address Generation**: HD wallet generates addresses with public keys
- ✅ **Current Address**: `GetCurrentAddress()` returns the most recent address
- ✅ **Public Key Access**: `addressInfo.PublicKey` contains the hex-encoded public key

#### Implementation Strategy:
1. **Use Current Address**: BRC-100 sites get the wallet's current address and public key
2. **No Session Storage**: Public key is derived from the current address, no need to store separately
3. **Consistent Response**: Always return the same public key for the current address
4. **HD Wallet Integration**: Leverage existing HD wallet infrastructure

#### Simple Implementation:
```go
func handleGetPublicKey(w http.ResponseWriter, r *http.Request) {
    // Get current wallet address (already has public key)
    addressInfo, err := walletService.walletManager.GetCurrentAddress()
    if err != nil {
        http.Error(w, fmt.Sprintf("Failed to get wallet address: %v", err), http.StatusInternalServerError)
        return
    }

    // Return the public key that's already stored
    response := map[string]interface{}{
        "publicKey": addressInfo.PublicKey,  // ✅ Already in hex format
        "address":   addressInfo.Address,    // ✅ Bitcoin address
        "index":     addressInfo.Index,      // ✅ Address index
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}
```

### Why This Approach Works:
1. **No Additional Storage**: Public key is already stored with each address
2. **Consistent Identity**: BRC-100 sites always get the same public key for the current address
3. **HD Wallet Compatible**: Works with existing HD wallet infrastructure
4. **Simple Implementation**: Minimal code changes required
5. **Secure**: Public key is derived from the wallet's master key

## Testing Strategy

### Unit Testing
- Test individual endpoint handlers
- Test domain whitelist functionality
- Test error handling and edge cases

### Integration Testing
- Test with real BRC-100 websites
- Test domain approval workflow
- Test authentication flow

### End-to-End Testing
- Complete user journey from site request to wallet access
- Test with multiple BRC-100 sites simultaneously
- Performance testing under load

## Security Considerations

### Domain Validation
- All BRC-100 requests must include valid Origin header
- Domain whitelist prevents unauthorized access
- User approval required for new domains

### Request Validation
- All endpoints validate HTTP method
- CORS headers properly configured
- Input validation on all request parameters

### Process Isolation
- BRC-100 server runs in separate Go daemon process
- No direct access to CEF browser process
- Secure communication via HTTP

## Timeline Estimate
- **Phase 1-2**: 1-2 days (Basic endpoints and port changes)
- **Phase 3-4**: 1-2 days (Domain integration and testing)
- **Phase 5**: 1-2 days (Authentication flow fixes)

**Total**: 3-6 days for complete implementation

## Success Metrics
- ✅ BRC-100 sites can discover wallet via `/getVersion`
- ✅ BRC-100 sites can get public key via `/getPublicKey`
- ✅ Domain whitelist prevents unauthorized access
- ✅ User approval modal works for new domains
- ✅ Authentication flow works with real BRC-100 sites
- ✅ No more "ERR_CONNECTION_REFUSED" errors
- ✅ No more "No wallet available" messages

## Conclusion

This implementation plan provides a structured approach to adding BRC-100 wallet integration to the Bitcoin Browser. The key insight is that our HD wallet already stores public keys with each address, making the implementation much simpler than initially anticipated.

The phased approach ensures security, compatibility, and maintainability while providing a clear path to full BRC-100 protocol support.
