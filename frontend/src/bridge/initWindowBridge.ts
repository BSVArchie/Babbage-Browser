
// Safely define the shell → native message bridge
if (!window.bitcoinBrowser) window.bitcoinBrowser = {} as any;

if (!window.bitcoinBrowser.navigation) {
  window.bitcoinBrowser.navigation = {
    navigate: (url: string) => {
      if (window.cefMessage?.send) {
        window.cefMessage.send('navigate', [url]);
      } else {
        console.warn('⚠️ cefMessage bridge not available');
      }
    }
  };
}

// Debug: Check what bitcoinBrowser.overlay looks like
console.log("🔍 Bridge: window.bitcoinBrowser:", window.bitcoinBrowser);
console.log("🔍 Bridge: window.bitcoinBrowser.overlay:", window.bitcoinBrowser?.overlay);
console.log("🔍 Bridge: typeof overlay:", typeof window.bitcoinBrowser?.overlay);

// Only set methods if they don't already exist (don't override injected methods)
if (!window.bitcoinBrowser.overlay?.show) {
  if (!window.bitcoinBrowser.overlay) {
    (window.bitcoinBrowser as any).overlay = {};
  }
  window.bitcoinBrowser.overlay.show = () => {
    console.log("🧠 JS: Sending overlay_show to native");
    console.log("Bridge is executing from URL:", window.location.href);
    window.cefMessage?.send('overlay_show', []);
  };
} else {
  // Check if this is our injected method (uses chrome.runtime.sendMessage)
  const methodString = window.bitcoinBrowser.overlay.show.toString();
  if (methodString.includes('chrome.runtime.sendMessage') && methodString.includes('test_overlay')) {
    console.log("🔍 Bridge: overlay.show is our injected method, not overriding");
  } else {
    console.log("🔍 Bridge: overlay.show exists but is not our injected method, not overriding");
  }
}

if (!window.bitcoinBrowser.overlay?.hide) {
  if (!window.bitcoinBrowser.overlay) {
    (window.bitcoinBrowser as any).overlay = {};
  }
  window.bitcoinBrowser.overlay.hide = () => window.cefMessage?.send?.('overlay_hide', []);
}

if (!window.bitcoinBrowser.overlay?.toggleInput) {
  if (!window.bitcoinBrowser.overlay) {
    (window.bitcoinBrowser as any).overlay = {};
  }
  window.bitcoinBrowser.overlay.toggleInput = (enable: boolean) =>
    window.cefMessage?.send?.('overlay_input', [enable]);
}

if (!window.bitcoinBrowser.overlay?.close) {
  if (!window.bitcoinBrowser.overlay) {
    (window.bitcoinBrowser as any).overlay = {};
  }
  window.bitcoinBrowser.overlay.close = () => {
    console.log("🧠 JS: Sending overlay_close to native");
    window.cefMessage?.send?.('overlay_close', []);
  };
}

if (!window.bitcoinBrowser.address) {
  window.bitcoinBrowser.address = {
    generate: () => {
      console.log("🔑 JS: Sending address_generate to native");
      return new Promise((resolve, reject) => {
        // Set up response handlers
        window.onAddressGenerated = (data: any) => {
          console.log("✅ Address generated:", data);
          resolve(data);
          delete window.onAddressGenerated;
          delete window.onAddressError;
        };

        window.onAddressError = (error: string) => {
          console.error("❌ Address generation error:", error);
          reject(new Error(error));
          delete window.onAddressGenerated;
          delete window.onAddressError;
        };

        // Send the request
        window.cefMessage?.send('address_generate', []);
      });
    }
  };
}

// overlayPanel methods removed - now using process-per-overlay architecture
