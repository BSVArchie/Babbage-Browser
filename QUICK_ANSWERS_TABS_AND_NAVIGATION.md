# Quick Answers: Tabs, Navigation, and Security

## Your Questions Answered

### Q1: How do our windows run their own processes?

**Answer**: Each window has its own **CEF browser instance** created with `CefBrowserHost::CreateBrowser()`:

```cpp
// Header browser - React UI
CefRefPtr<SimpleHandler> header_handler = new SimpleHandler("header");
CefBrowserHost::CreateBrowser(..., header_handler, "http://127.0.0.1:5137", ...);

// Webview browser - Web content
CefRefPtr<SimpleHandler> webview_handler = new SimpleHandler("webview");
CefBrowserHost::CreateBrowser(..., webview_handler, "https://metanetapps.com/", ...);

// Settings overlay - Separate process
CefRefPtr<SimpleHandler> settings_handler = new SimpleHandler("settings");
CefBrowserHost::CreateBrowser(..., settings_handler, "http://127.0.0.1:5137/settings", ...);
```

**Each browser gets:**
- ✅ Own render process
- ✅ Own V8 JavaScript engine
- ✅ Own memory space
- ✅ Identified by `role_` string ("header", "webview", "settings", etc.)

---

### Q2: What do we need to add tabs with process-per-tab?

**Answer**: 5 main components:

**1. Tab Data Structure**
```cpp
struct Tab {
    int id;
    HWND hwnd;
    CefRefPtr<CefBrowser> browser;
    std::string url;
    std::string title;
    bool isActive;
};

std::vector<Tab> tabs_;
```

**2. Tab Manager**
```cpp
class TabManager {
    int CreateTab(std::string url);    // Create new tab
    void CloseTab(int tabId);          // Close tab
    void SwitchToTab(int tabId);       // Switch active tab
    Tab* GetActiveTab();               // Get current tab
};
```

**3. Multiple HWNDs (Show/Hide on Switch)**
```cpp
// Create HWND for each tab
HWND tab1_hwnd = CreateWindow(..., WS_CHILD, ...);  // Tab 1
HWND tab2_hwnd = CreateWindow(..., WS_CHILD, ...);  // Tab 2

// Switch tabs
ShowWindow(tab1_hwnd, SW_HIDE);   // Hide Tab 1
ShowWindow(tab2_hwnd, SW_SHOW);   // Show Tab 2
```

**4. React Tab Bar UI**
```tsx
<TabBar>
    <Tab active={true}>Site 1</Tab>
    <Tab active={false}>Site 2</Tab>
    <IconButton onClick={createNewTab}>+</IconButton>
</TabBar>
```

**5. Message Protocol**
```cpp
// Include tab ID in messages
"tab_create" -> CreateTab()
"tab_close" -> CloseTab(tabId)
"tab_switch" -> SwitchToTab(tabId)
"tab_navigate" -> tab->browser->LoadURL(url)
```

---

### Q3: How will tabs affect wallet & BRC100 functionality?

**Answer**: ✅ **They work independently per tab!**

**Wallet API:**
- Each tab process gets `bitcoinBrowser` API injected
- Each tab makes HTTP requests to Go daemon independently
- Go daemon handles requests from all tabs concurrently
- **Works perfectly!**

**BRC100 Authentication:**
```
Tab 1 (peerpay.com):
- Requests auth ✅
- User approves ✅
- Gets session ✅
- Can make wallet operations ✅

Tab 2 (thryll.com):
- Requests auth ✅
- User approves ✅
- Gets DIFFERENT session ✅
- Can make wallet operations ✅

Both work simultaneously with no conflicts!
```

**Domain Whitelisting:**
- HTTP interceptor checks each request's domain
- Works per-request, not per-process
- Each tab's requests checked independently
- **Works perfectly!**

**Potential Issues:**
1. ⚠️ **UTXO double-spend** - Two tabs try to use same UTXO
   - **Fix**: Go daemon tracks used UTXOs, returns error if already used

2. ⚠️ **Multiple auth modals** - Two tabs request auth simultaneously
   - **Fix**: Queue auth requests, show one at a time

3. ⚠️ **Tab context** - Which tab gets the response?
   - **Fix**: Include tab ID in auth request, route response back

**Verdict**: ✅ Minor changes needed to Go daemon, but **fundamentally compatible**

---

### Q4: Should we implement back/forward/refresh before tabs?

**Answer**: ✅ **ABSOLUTELY YES!**

**Why Do Back/Forward/Refresh First:**

1. **Essential Features** ⭐⭐⭐⭐⭐
   - Users need these immediately
   - Basic browser functionality
   - Frustrating without them

2. **Quick Win** ⚡
   - 2-3 hours to implement
   - Already partially implemented in NavigationHandler
   - Just need to connect to React UI

3. **No Architecture Changes** 🏗️
   - Works with single webview
   - No process model changes
   - Tests existing system

4. **Foundation for Tabs** 📚
   - Understanding navigation helps with tabs
   - Same concepts apply to tab navigation
   - Less to learn when building tabs

5. **Low Risk** ✅
   - Simple CEF API calls
   - No new processes
   - Easy to test

**Why Tabs Can Wait:**

1. **Complex** - 2-3 weeks of work
2. **Not critical** - Single tab works fine for now
3. **Architecture decision** - Need to design carefully
4. **Testing required** - Need to validate security thoroughly

**Verdict**: ✅ **Do navigation buttons this week, tabs next month**

---

## 🚀 Implementation Plan Summary

### This Week (2-3 hours):
1. ✅ Back button
2. ✅ Forward button
3. ✅ Refresh button
4. ✅ URL bar updates on navigation

### Next Week (Design Phase):
1. Design tab architecture
2. Plan security model
3. Review CEF tab patterns
4. Create implementation plan

### Week 3-4 (Implementation):
1. Build TabManager
2. Implement multi-HWND layout
3. Create React tab bar
4. Test thoroughly

### Week 5 (Testing & Polish):
1. Test wallet operations with tabs
2. Test BRC100 auth with tabs
3. Security validation
4. Performance optimization

---

## 📊 Effort Comparison

| Feature | Time | Complexity | Priority | Value |
|---------|------|------------|----------|-------|
| **Back/Forward/Refresh** | 2-3 hours | ⭐ Low | ⭐⭐⭐⭐⭐ Critical | Immediate |
| **Tabs (Process-Per-Tab)** | 2-3 weeks | ⭐⭐⭐⭐ High | ⭐⭐⭐ Medium | Long-term |

---

## 🎯 Final Recommendation

**DO THIS:**
1. Implement back/forward/refresh buttons **THIS WEEK** ✅
2. Design tab architecture **NEXT WEEK** 📐
3. Implement tabs with process-per-tab **WEEK 3-4** 🏗️
4. Test & validate security **WEEK 5** 🔒

**DON'T DO THIS:**
- ❌ Implement tabs without process isolation (security risk)
- ❌ Skip navigation buttons (users need them now)
- ❌ Rush tab implementation (complex, needs careful design)

---

**Ready to implement back/forward/refresh buttons?** It's the smart next step! 🚀
