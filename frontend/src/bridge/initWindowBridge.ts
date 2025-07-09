
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
