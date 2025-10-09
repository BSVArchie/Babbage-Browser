# CEF + React Integration Guide

## 🎯 Overview

This guide explains how to properly integrate React applications with Chromium Embedded Framework (CEF) browser windows, with specific focus on window sizing, layout, and resize handling.

## 📊 Architecture

### Window Hierarchy
```
┌─────────────────────────────────────────────────────────┐
│              Main Shell Window (g_hwnd)                 │
│  ┌───────────────────────────────────────────────────┐  │
│  │   Header Window (g_header_hwnd) - 80px height    │  │
│  │   ┌─────────────────────────────────────────────┐ │  │
│  │   │   CEF Browser (React App runs here)         │ │  │
│  │   └─────────────────────────────────────────────┘ │  │
│  └───────────────────────────────────────────────────┘  │
│  ┌───────────────────────────────────────────────────┐  │
│  │   WebView Window (g_webview_hwnd) - fills rest   │  │
│  │   ┌─────────────────────────────────────────────┐ │  │
│  │   │   CEF Browser (Web content loads here)      │ │  │
│  │   └─────────────────────────────────────────────┘ │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 🔧 Key Issues Fixed

### Issue #1: React Components Not Fitting Window

**Problem**: React components were centered instead of filling the CEF window.

**Root Cause**: Default Vite React template CSS uses flexbox centering:
```css
body {
  display: flex;
  place-items: center;
  min-height: 100vh;
}
```

**Solution**: Override with full-window layout CSS:
```css
html, body {
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100%;
  overflow: hidden;
}

#root {
  width: 100%;
  height: 100%;
  margin: 0;
  padding: 0;
  overflow: hidden;
}
```

### Issue #2: Window Not Resizing Properly

**Problem**: When the main window is resized, CEF browsers don't resize.

**Root Cause**: No `WM_SIZE` handler in `ShellWindowProc` to propagate resize events.

**Solution**: Add comprehensive `WM_SIZE` handler:
```cpp
case WM_SIZE: {
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Resize child windows
    SetWindowPos(g_header_hwnd, nullptr, 0, 0, width, shellHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);

    // Notify CEF browser of resize
    CefRefPtr<CefBrowser> browser = SimpleHandler::GetHeaderBrowser();
    if (browser) {
        browser->GetHost()->WasResized();
    }

    return 0;
}
```

### Issue #3: React Components Not Using Full Height

**Problem**: React components like `MainBrowserView` don't fill available space.

**Solution**: Add explicit sizing to React components:
```tsx
<Box
    sx={{
        width: '100%',
        height: '100%',
        display: 'flex',
        flexDirection: 'column',
        overflow: 'hidden'
    }}
>
    {/* Content */}
</Box>
```

## 📋 Best Practices

### 1. CSS for CEF Windows

**Always use full-window layout CSS:**
```css
/* index.css */
html, body, #root {
  margin: 0;
  padding: 0;
  width: 100%;
  height: 100%;
  overflow: hidden; /* Prevent scrollbars */
}
```

**Avoid these patterns in CEF:**
- ❌ `display: flex` with `place-items: center` on body
- ❌ `max-width` constraints on root elements
- ❌ `margin: 0 auto` for centering
- ❌ `min-height: 100vh` (use `height: 100%` instead)

### 2. React Component Sizing

**Root-level components should fill window:**
```tsx
// MainBrowserView.tsx
return (
    <Box
        sx={{
            width: '100%',
            height: '100%',
            display: 'flex',
            flexDirection: 'column'
        }}
    >
        {/* Content */}
    </Box>
);
```

**Use flexbox for layout:**
```tsx
<Box sx={{ display: 'flex', flexDirection: 'column', height: '100%' }}>
    <Toolbar sx={{ flexShrink: 0 }}>Header</Toolbar>
    <Box sx={{ flex: 1, overflow: 'auto' }}>Content</Box>
</Box>
```

### 3. CEF Window Management

**Always handle WM_SIZE:**
```cpp
case WM_SIZE: {
    // 1. Get new dimensions
    RECT rect;
    GetClientRect(hwnd, &rect);

    // 2. Resize child windows
    SetWindowPos(child_hwnd, nullptr, x, y, width, height,
        SWP_NOZORDER | SWP_NOACTIVATE);

    // 3. Notify CEF browsers
    browser->GetHost()->WasResized();

    return 0;
}
```

**Create CEF browsers with proper sizing:**
```cpp
CefWindowInfo window_info;
window_info.SetAsChild(parent_hwnd, CefRect(0, 0, width, height));

CefBrowserSettings settings;
CefBrowserHost::CreateBrowser(window_info, handler, url, settings, nullptr, nullptr);
```

### 4. Common Pitfalls

**❌ Don't use viewport units in CEF:**
```css
/* BAD - viewport units can cause issues */
.container {
    height: 100vh;
    width: 100vw;
}

/* GOOD - use percentage-based sizing */
.container {
    height: 100%;
    width: 100%;
}
```

**❌ Don't forget overflow handling:**
```css
/* BAD - can cause unwanted scrollbars */
body {
    overflow: auto;
}

/* GOOD - control overflow explicitly */
body {
    overflow: hidden;
}
.scrollable-content {
    overflow-y: auto;
}
```

**❌ Don't use fixed pixel dimensions at root level:**
```tsx
// BAD - fixed dimensions don't resize
<Box sx={{ width: 1280, height: 720 }}>

// GOOD - use percentage or flex
<Box sx={{ width: '100%', height: '100%' }}>
```

## 🚀 Implementation Checklist

When integrating React with CEF, follow this checklist:

### Frontend (React)
- [ ] Remove centering CSS from `index.css` (no flexbox centering on body)
- [ ] Set `html`, `body`, `#root` to `width: 100%`, `height: 100%`
- [ ] Add `overflow: hidden` to prevent scrollbars
- [ ] Use flexbox layout in root components
- [ ] Test with different window sizes
- [ ] Verify no scrollbars appear

### Backend (C++)
- [ ] Implement `WM_SIZE` handler in window procedure
- [ ] Resize child windows with `SetWindowPos`
- [ ] Call `browser->GetHost()->WasResized()` after resize
- [ ] Handle both header and webview windows
- [ ] Test window maximize/restore/resize
- [ ] Verify CEF browsers update correctly

### Testing
- [ ] Test window resize (drag corners)
- [ ] Test maximize/restore
- [ ] Test on different screen resolutions
- [ ] Verify no visual artifacts during resize
- [ ] Check for memory leaks during repeated resizing
- [ ] Test with multiple monitors

## 🔍 Debugging Tips

### Check if React is filling window:
```javascript
// In browser console
console.log('HTML height:', document.documentElement.clientHeight);
console.log('Body height:', document.body.clientHeight);
console.log('Root height:', document.getElementById('root').clientHeight);
// All should match the window height
```

### Check CEF browser dimensions:
```cpp
// In C++ code
RECT rect;
GetClientRect(browser->GetHost()->GetWindowHandle(), &rect);
LOG_DEBUG("CEF Browser size: " +
    std::to_string(rect.right) + "x" +
    std::to_string(rect.bottom));
```

### Common Issues:

**Issue**: React content is centered with empty space
- **Fix**: Remove flexbox centering from CSS

**Issue**: Window resize doesn't update CEF browser
- **Fix**: Add `WM_SIZE` handler with `WasResized()` call

**Issue**: Scrollbars appear unnecessarily
- **Fix**: Add `overflow: hidden` to parent elements

**Issue**: React components don't fill height
- **Fix**: Ensure parent elements have `height: 100%` all the way up

## 📚 Additional Resources

### CEF Documentation
- [CEF Browser Sizing](https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage.md#markdown-header-browser-size)
- [Window Management](https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage.md#markdown-header-window-management)
- [Off-Screen Rendering](https://bitbucket.org/chromiumembedded/cef/wiki/GeneralUsage.md#markdown-header-off-screen-rendering)

### React Best Practices
- [React Layout Patterns](https://react.dev/learn/preserving-and-resetting-state)
- [Flexbox Layout Guide](https://css-tricks.com/snippets/css/a-guide-to-flexbox/)

### Windows API
- [SetWindowPos](https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
- [WM_SIZE Message](https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-size)

## 🎉 Summary

**Key Takeaways:**
1. **CSS**: Always use `width: 100%`, `height: 100%`, never use centering flexbox on body
2. **Windows**: Always handle `WM_SIZE` and call `browser->GetHost()->WasResized()`
3. **React**: Use flexbox for layout, ensure root components fill window
4. **Testing**: Test all window resize scenarios thoroughly

**Files Modified:**
- `frontend/src/index.css` - Full-window layout CSS
- `frontend/src/App.css` - App container styling
- `frontend/src/pages/MainBrowserView.tsx` - Component sizing
- `cef-native/cef_browser_shell.cpp` - WM_SIZE handler

With these changes, your React application will properly fit CEF browser windows and resize smoothly with the main window!
