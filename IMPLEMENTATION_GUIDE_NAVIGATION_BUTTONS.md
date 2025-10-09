# Step-by-Step Implementation: Back/Forward/Refresh Buttons

**Estimated Time**: 2-3 hours
**Difficulty**: ⭐ Easy
**Priority**: ⭐⭐⭐⭐⭐ Critical

## 🎯 Goal

Add functional back, forward, and refresh buttons to the browser navigation bar.

## 📋 Prerequisites Checklist

✅ NavigationHandler exists (`cef-native/src/core/NavigationHandler.cpp`)
✅ useBitcoinBrowser hook exists (`frontend/src/hooks/useBitcoinBrowser.ts`)
✅ MainBrowserView has placeholder buttons (`frontend/src/pages/MainBrowserView.tsx`)
✅ CEF tracks canGoBack/canGoForward state

## 🔧 Step-by-Step Implementation

---

## **STEP 1: Add C++ Message Handlers (Backend)**

**File**: `cef-native/src/handlers/simple_handler.cpp`

**Location**: In the `OnProcessMessageReceived` method, find where navigation messages are handled (search for `message_name == "navigate"`), and add these new handlers nearby:

**Add this code:**

```cpp
// Add these handlers near the existing "navigate" handler (around line 700-800)

if (message_name == "navigate_back") {
    LOG_DEBUG_BROWSER("🔙 navigate_back message received from role: " + role_);

    CefRefPtr<CefBrowser> webview = SimpleHandler::GetWebviewBrowser();
    if (webview) {
        webview->GoBack();
        LOG_DEBUG_BROWSER("🔙 GoBack() called on webview browser");
    } else {
        LOG_WARNING_BROWSER("⚠️ No webview browser available for GoBack");
    }
    return true;
}

if (message_name == "navigate_forward") {
    LOG_DEBUG_BROWSER("🔜 navigate_forward message received from role: " + role_);

    CefRefPtr<CefBrowser> webview = SimpleHandler::GetWebviewBrowser();
    if (webview) {
        webview->GoForward();
        LOG_DEBUG_BROWSER("🔜 GoForward() called on webview browser");
    } else {
        LOG_WARNING_BROWSER("⚠️ No webview browser available for GoForward");
    }
    return true;
}

if (message_name == "navigate_reload") {
    LOG_DEBUG_BROWSER("🔄 navigate_reload message received from role: " + role_);

    CefRefPtr<CefBrowser> webview = SimpleHandler::GetWebviewBrowser();
    if (webview) {
        webview->Reload();
        LOG_DEBUG_BROWSER("🔄 Reload() called on webview browser");
    } else {
        LOG_WARNING_BROWSER("⚠️ No webview browser available for Reload");
    }
    return true;
}
```

**Why**: These handlers receive messages from React and call CEF's built-in navigation methods.

**Testing**: After adding, rebuild and check `debug_output.log` for these messages when clicking buttons.

---

## **STEP 2: Update React Hook (Frontend)**

**File**: `frontend/src/hooks/useBitcoinBrowser.ts`

**Location**: Inside the `useBitcoinBrowser()` function, after the existing `navigate` function (around line 78)

**Add these new functions:**

```typescript
const goBack = useCallback((): void => {
    console.log('🔙 Going back in browser history');
    window.cefMessage?.send('navigate_back', []);
}, []);

const goForward = useCallback((): void => {
    console.log('🔜 Going forward in browser history');
    window.cefMessage?.send('navigate_forward', []);
}, []);

const reload = useCallback((): void => {
    console.log('🔄 Reloading current page');
    window.cefMessage?.send('navigate_reload', []);
}, []);
```

**Update the return statement** (around line 80-85):

```typescript
return {
    getIdentity,
    markBackedUp,
    generateAddress,
    navigate,
    goBack,      // ← Add this
    goForward,   // ← Add this
    reload,      // ← Add this
};
```

**Why**: These functions send CEF messages to trigger navigation in the webview browser.

**Testing**: Open browser console, call `window.bitcoinBrowser.navigation.goBack()` manually to test.

---

## **STEP 3: Wire Up React Buttons (Frontend)**

**File**: `frontend/src/pages/MainBrowserView.tsx`

**Location**: Import the new functions and connect to button onClick handlers

**Step 3a: Update the destructuring** (around line 31):

```typescript
// Change from:
const { navigate } = useBitcoinBrowser();

// Change to:
const { navigate, goBack, goForward, reload } = useBitcoinBrowser();
```

**Step 3b: Add Refresh icon import** (top of file, around line 1-14):

```typescript
import RefreshIcon from '@mui/icons-material/Refresh';
```

**Step 3c: Connect buttons to handlers** (around lines 48-53):

```typescript
{/* Back Button */}
<IconButton onClick={goBack}>
    <ArrowBackIcon />
</IconButton>

{/* Forward Button */}
<IconButton onClick={goForward}>
    <ArrowForwardIcon />
</IconButton>
```

**Step 3d: Add Refresh button** (after forward button, before address bar):

```typescript
{/* Refresh Button */}
<IconButton onClick={reload}>
    <RefreshIcon />
</IconButton>
```

**Why**: Connects UI buttons to the navigation functions.

**Testing**: Click buttons in UI and verify webview navigates.

---

## **STEP 4: Optional - Enable/Disable Buttons Based on State**

**Enhancement**: Disable back button when can't go back, disable forward when can't go forward.

**File**: `frontend/src/pages/MainBrowserView.tsx`

**Add state tracking:**

```typescript
const [canGoBack, setCanGoBack] = useState(false);
const [canGoForward, setCanGoForward] = useState(false);

useEffect(() => {
    // Listen for navigation state updates from CEF
    const handleNavState = (event: CustomEvent) => {
        if (event.detail.message === 'navigation_state_update') {
            const state = JSON.parse(event.detail.args[0]);
            setCanGoBack(state.canGoBack);
            setCanGoForward(state.canGoForward);
        }
    };

    window.addEventListener('cefMessageResponse', handleNavState as any);
    return () => window.removeEventListener('cefMessageResponse', handleNavState as any);
}, []);
```

**Update buttons:**

```typescript
<IconButton onClick={goBack} disabled={!canGoBack}>
    <ArrowBackIcon />
</IconButton>

<IconButton onClick={goForward} disabled={!canGoForward}>
    <ArrowForwardIcon />
</IconButton>
```

**Backend support** (in `simple_handler.cpp`, in `OnLoadingStateChange`):

```cpp
// Around line 150-165, after logging canGoBack/canGoForward
if (role_ == "webview") {
    // Send navigation state to header browser for UI updates
    CefRefPtr<CefBrowser> header = SimpleHandler::GetHeaderBrowser();
    if (header && header->GetMainFrame()) {
        std::string navState = "{\"canGoBack\":" + std::string(canGoBack ? "true" : "false") +
                              ",\"canGoForward\":" + std::string(canGoForward ? "true" : "false") + "}";
        std::string js = "window.dispatchEvent(new CustomEvent('cefMessageResponse', {detail: {message: 'navigation_state_update', args: ['" + navState + "']}}))";
        header->GetMainFrame()->ExecuteJavaScript(js, header->GetMainFrame()->GetURL(), 0);
    }
}
```

**Why**: Better UX - users see which navigation actions are available.

**Testing**: Navigate to a page, verify back button is disabled. Click a link, verify back button enables.

---

## **STEP 5: Optional - Update URL Bar on Navigation**

**Enhancement**: Address bar shows current page URL.

**File**: `frontend/src/pages/MainBrowserView.tsx`

**Add effect to listen for URL changes:**

```typescript
useEffect(() => {
    // Listen for URL updates from webview
    const handleUrlUpdate = (event: CustomEvent) => {
        if (event.detail.message === 'url_changed') {
            const newUrl = event.detail.args[0];
            setAddress(newUrl);
            console.log('🌐 URL updated to:', newUrl);
        }
    };

    window.addEventListener('cefMessageResponse', handleUrlUpdate as any);
    return () => window.removeEventListener('cefMessageResponse', handleUrlUpdate as any);
}, []);
```

**Backend support** (in `simple_handler.cpp`, in `OnLoadingStateChange`):

```cpp
if (role_ == "webview" && !isLoading) {
    // Send URL update to header browser
    CefRefPtr<CefBrowser> header = SimpleHandler::GetHeaderBrowser();
    if (header && header->GetMainFrame()) {
        std::string currentUrl = browser->GetMainFrame()->GetURL().ToString();
        std::string js = "window.dispatchEvent(new CustomEvent('cefMessageResponse', {detail: {message: 'url_changed', args: ['" + currentUrl + "']}}))";
        header->GetMainFrame()->ExecuteJavaScript(js, header->GetMainFrame()->GetURL(), 0);
    }
}
```

**Why**: Address bar stays in sync with current page.

**Testing**: Navigate using links on page, verify address bar updates automatically.

---

## 📝 Complete Code Checklist

### Files to Modify:

**Backend (C++):**
- [x] `cef-native/src/handlers/simple_handler.cpp` - Add 3 message handlers (navigate_back, navigate_forward, navigate_reload)
- [ ] Optional: Add navigation state updates in `OnLoadingStateChange`

**Frontend (TypeScript/React):**
- [x] `frontend/src/hooks/useBitcoinBrowser.ts` - Add goBack, goForward, reload functions
- [x] `frontend/src/pages/MainBrowserView.tsx` - Import RefreshIcon, wire up onClick handlers
- [ ] Optional: Add state tracking for button enable/disable
- [ ] Optional: Add URL bar sync

---

## 🧪 Testing Plan

### Basic Functionality Tests

**Test 1: Back Button**
1. Start app (loads metanetapps.com)
2. Click link to go to another page
3. Click back button
4. **Expected**: Returns to previous page ✅

**Test 2: Forward Button**
1. After going back (Test 1)
2. Click forward button
3. **Expected**: Returns to page you went back from ✅

**Test 3: Refresh Button**
1. Navigate to any page
2. Scroll down or interact with page
3. Click refresh button
4. **Expected**: Page reloads from beginning ✅

### Advanced Tests

**Test 4: Back Button Disabled**
1. Fresh app start (no history yet)
2. **Expected**: Back button should be disabled (if you implemented Step 4) ✅

**Test 5: URL Bar Updates**
1. Navigate to page A
2. Click link to page B
3. **Expected**: Address bar shows page B URL (if you implemented Step 5) ✅

**Test 6: Refresh During Load**
1. Start loading slow page
2. Click refresh while loading
3. **Expected**: Cancels first load, starts fresh load ✅

---

## 🎯 Minimal Implementation (Core Features Only)

**If you want the FASTEST implementation**, do only these 3 steps:

### Minimal Step 1: Backend (15 minutes)

Add to `simple_handler.cpp`:

```cpp
if (message_name == "navigate_back") {
    if (webview_browser_) webview_browser_->GoBack();
    return true;
}
if (message_name == "navigate_forward") {
    if (webview_browser_) webview_browser_->GoForward();
    return true;
}
if (message_name == "navigate_reload") {
    if (webview_browser_) webview_browser_->Reload();
    return true;
}
```

### Minimal Step 2: Frontend Hook (10 minutes)

Add to `useBitcoinBrowser.ts`:

```typescript
const goBack = useCallback(() => window.cefMessage?.send('navigate_back', []), []);
const goForward = useCallback(() => window.cefMessage?.send('navigate_forward', []), []);
const reload = useCallback(() => window.cefMessage?.send('navigate_reload', []), []);

return { getIdentity, markBackedUp, generateAddress, navigate, goBack, goForward, reload };
```

### Minimal Step 3: Wire Up Buttons (10 minutes)

In `MainBrowserView.tsx`:

```typescript
// Add import
import RefreshIcon from '@mui/icons-material/Refresh';

// Add to component
const { navigate, goBack, goForward, reload } = useBitcoinBrowser();

// Update buttons
<IconButton onClick={goBack}><ArrowBackIcon /></IconButton>
<IconButton onClick={goForward}><ArrowForwardIcon /></IconButton>
<IconButton onClick={reload}><RefreshIcon /></IconButton>
```

**Total Time**: ~35 minutes for basic working buttons!

---

## 🚀 Full Implementation (With Polish)

**If you want proper UX**, add Steps 4-5 for:
- Disabled state on buttons when can't navigate
- URL bar sync
- Visual feedback

**Total Time**: ~2-3 hours including testing and polish

---

## 📊 Implementation Phases

### Phase 1: Core Functionality (35 minutes) ✅ **DO THIS FIRST**

- Step 1: Add 3 message handlers to C++
- Step 2: Add 3 functions to React hook
- Step 3: Wire up button onClick handlers

**Result**: Working back/forward/refresh buttons

### Phase 2: UX Polish (1 hour) ⭐ **RECOMMENDED**

- Step 4: Add button enabled/disabled state
- Add visual feedback (button color/cursor)
- Test edge cases

**Result**: Professional browser navigation

### Phase 3: Advanced Features (1 hour) 🌟 **NICE TO HAVE**

- Step 5: URL bar sync
- Navigation history tracking
- Keyboard shortcuts (Alt+Left, Alt+Right, F5)

**Result**: Full-featured browser navigation

---

## 🎯 Quick Start Guide (Get It Working Fast)

### 1. Open These Files:

```
cef-native/src/handlers/simple_handler.cpp
frontend/src/hooks/useBitcoinBrowser.ts
frontend/src/pages/MainBrowserView.tsx
```

### 2. Copy-Paste Code:

**Into `simple_handler.cpp`** (around line 700, near "navigate" handler):
```cpp
if (message_name == "navigate_back") {
    if (webview_browser_) webview_browser_->GoBack();
    return true;
}
if (message_name == "navigate_forward") {
    if (webview_browser_) webview_browser_->GoForward();
    return true;
}
if (message_name == "navigate_reload") {
    if (webview_browser_) webview_browser_->Reload();
    return true;
}
```

**Into `useBitcoinBrowser.ts`** (around line 78, before return statement):
```typescript
const goBack = useCallback(() => window.cefMessage?.send('navigate_back', []), []);
const goForward = useCallback(() => window.cefMessage?.send('navigate_forward', []), []);
const reload = useCallback(() => window.cefMessage?.send('navigate_reload', []), []);
```

**Update return statement**:
```typescript
return { getIdentity, markBackedUp, generateAddress, navigate, goBack, goForward, reload };
```

**Into `MainBrowserView.tsx`**:

Add import (top of file):
```typescript
import RefreshIcon from '@mui/icons-material/Refresh';
```

Update destructuring (around line 31):
```typescript
const { navigate, goBack, goForward, reload } = useBitcoinBrowser();
```

Update buttons (around lines 48-53):
```typescript
<IconButton onClick={goBack}>
    <ArrowBackIcon />
</IconButton>
<IconButton onClick={goForward}>
    <ArrowForwardIcon />
</IconButton>
<IconButton onClick={reload}>
    <RefreshIcon />
</IconButton>
```

### 3. Build & Test:

```powershell
# In frontend directory
cd frontend
npm run dev

# In Visual Studio or CMake
# Build the C++ project

# Run the application
# Test: Navigate to a page, click a link, then click back button
```

---

## 🐛 Troubleshooting

### Issue: Buttons Don't Do Anything

**Check:**
1. Did you rebuild the C++ project after adding handlers?
2. Check `debug_output.log` - do you see "navigate_back message received"?
3. Check browser console - do you see "🔙 Going back in browser history"?

**Debug:**
```typescript
// In MainBrowserView.tsx, add console logs
onClick={() => {
    console.log('Back button clicked');
    goBack();
}}
```

### Issue: "webview_browser_ is undefined"

**Check:**
- Wait for page to load fully before clicking
- Check logs for "📡 WebView browser reference stored"

**Fix:**
- Add null check in C++ (already included in code above)

### Issue: TypeScript Error on useBitcoinBrowser

**Fix**: TypeScript might not recognize new functions. Add type:

```typescript
// In frontend/src/types/window.d.ts or at top of useBitcoinBrowser.ts
interface NavigationFunctions {
    navigate: (path: string) => void;
    goBack: () => void;
    goForward: () => void;
    reload: () => void;
}
```

---

## ✅ Success Criteria

After implementation, you should be able to:

- ✅ Click back button to go to previous page
- ✅ Click forward button to go to next page (after going back)
- ✅ Click refresh button to reload current page
- ✅ See navigation working smoothly
- ✅ See debug messages in logs confirming navigation

---

## 📚 CEF API Reference

### Methods We're Using:

```cpp
browser->GoBack()     // Navigate to previous page in history
browser->GoForward()  // Navigate to next page in history
browser->Reload()     // Reload current page
```

### Navigation State Methods:

```cpp
browser->CanGoBack()     // Returns true if history has previous page
browser->CanGoForward()  // Returns true if history has next page
browser->IsLoading()     // Returns true if page is currently loading
```

**Already available** in `OnLoadingStateChange` callback as parameters!

---

## 🎨 Optional UI Improvements

### Keyboard Shortcuts

```typescript
useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
        if (e.altKey && e.key === 'ArrowLeft') {
            e.preventDefault();
            goBack();
        }
        if (e.altKey && e.key === 'ArrowRight') {
            e.preventDefault();
            goForward();
        }
        if (e.key === 'F5') {
            e.preventDefault();
            reload();
        }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
}, [goBack, goForward, reload]);
```

### Visual Feedback

```typescript
<IconButton
    onClick={goBack}
    disabled={!canGoBack}
    sx={{
        '&:disabled': {
            opacity: 0.3,
            cursor: 'not-allowed'
        }
    }}
>
    <ArrowBackIcon />
</IconButton>
```

### Loading Indicator

```typescript
const [isLoading, setIsLoading] = useState(false);

// Update based on webview loading state
// Show spinner on refresh button when loading

<IconButton onClick={reload}>
    {isLoading ? <CircularProgress size={24} /> : <RefreshIcon />}
</IconButton>
```

---

## 🎯 Summary

### Minimal Implementation (35 minutes):
- 3 message handlers in C++ (Step 1)
- 3 functions in React hook (Step 2)
- Wire up 3 buttons (Step 3)
- **Result**: Working navigation ✅

### Full Implementation (2-3 hours):
- Minimal implementation
- Button enabled/disabled state (Step 4)
- URL bar sync (Step 5)
- Visual polish
- Keyboard shortcuts
- **Result**: Professional browser navigation ✅

---

**Ready to start? Begin with Step 1 - add the C++ message handlers!** 🚀
