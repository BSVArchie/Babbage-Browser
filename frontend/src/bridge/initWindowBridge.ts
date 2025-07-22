
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

if (!window.bitcoinBrowser.overlay) {
  window.bitcoinBrowser.overlay = {
    // show: () => window.cefMessage?.send?.('overlay_show', []),
    show: () => {
      console.log("🧠 JS: Sending overlay_show to native");
      console.log("Bridge is executing from URL:", window.location.href);
      window.cefMessage?.send('overlay_show', []);
    },
    hide: () => window.cefMessage?.send?.('overlay_hide', []),
    toggleInput: (enable: boolean) =>
      window.cefMessage?.send?.('overlay_input', [enable]),
  };
}

