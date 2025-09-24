
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

console.log("🔍 initWindowBridge: Setting up bitcoinBrowser.address");
console.log("🔍 initWindowBridge: window.bitcoinBrowser.address exists:", !!window.bitcoinBrowser.address);

// Force override the existing function
console.log("🔍 initWindowBridge: Forcing override of address.generate function");
window.bitcoinBrowser.address.generate = () => {
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
};

        // Transaction methods
        if (!window.bitcoinAPI) {
          window.bitcoinAPI = {
            createTransaction: (data: any) => {
      console.log("💰 JS: Sending create_transaction to native");
      return new Promise((resolve, reject) => {
        window.onCreateTransactionResponse = (data: any) => {
          console.log("✅ Transaction created:", data);
          resolve(data);
          delete window.onCreateTransactionResponse;
          delete window.onCreateTransactionError;
        };

        window.onCreateTransactionError = (error: string) => {
          console.error("❌ Transaction creation error:", error);
          reject(new Error(error));
          delete window.onCreateTransactionResponse;
          delete window.onCreateTransactionError;
        };

        window.cefMessage?.send('create_transaction', [JSON.stringify(data)]);
      });
    },

    signTransaction: (data: any) => {
      console.log("✍️ JS: Sending sign_transaction to native");
      return new Promise((resolve, reject) => {
        window.onSignTransactionResponse = (data: any) => {
          console.log("✅ Transaction signed:", data);
          resolve(data);
          delete window.onSignTransactionResponse;
          delete window.onSignTransactionError;
        };

        window.onSignTransactionError = (error: string) => {
          console.error("❌ Transaction signing error:", error);
          reject(new Error(error));
          delete window.onSignTransactionResponse;
          delete window.onSignTransactionError;
        };

        window.cefMessage?.send('sign_transaction', [JSON.stringify(data)]);
      });
    },

    broadcastTransaction: (data: any) => {
      console.log("📡 JS: Sending broadcast_transaction to native");
      return new Promise((resolve, reject) => {
        window.onBroadcastTransactionResponse = (data: any) => {
          console.log("✅ Transaction broadcasted:", data);
          resolve(data);
          delete window.onBroadcastTransactionResponse;
          delete window.onBroadcastTransactionError;
        };

        window.onBroadcastTransactionError = (error: string) => {
          console.error("❌ Transaction broadcast error:", error);
          reject(new Error(error));
          delete window.onBroadcastTransactionResponse;
          delete window.onBroadcastTransactionError;
        };

        window.cefMessage?.send('broadcast_transaction', [JSON.stringify(data)]);
      });
    },

    getBalance: (data: any) => {
      console.log("💳 JS: Sending get_balance to native");
      console.log("💳 JS: Data being sent:", data);
      console.log("💳 JS: JSON.stringify(data):", JSON.stringify(data));
      return new Promise((resolve, reject) => {
        window.onGetBalanceResponse = (data: any) => {
          console.log("✅ Balance retrieved:", data);
          resolve(data);
          delete window.onGetBalanceResponse;
          delete window.onGetBalanceError;
        };

        window.onGetBalanceError = (error: string) => {
          console.error("❌ Balance retrieval error:", error);
          reject(new Error(error));
          delete window.onGetBalanceResponse;
          delete window.onGetBalanceError;
        };

        window.cefMessage?.send('get_balance', [JSON.stringify(data)]);
      });
    },

    getTransactionHistory: () => {
      console.log("📜 JS: Sending get_transaction_history to native");
      return new Promise((resolve, reject) => {
        window.onGetTransactionHistoryResponse = (data: any) => {
          console.log("✅ Transaction history retrieved:", data);
          resolve(data);
          delete window.onGetTransactionHistoryResponse;
          delete window.onGetTransactionHistoryError;
        };

        window.onGetTransactionHistoryError = (error: string) => {
          console.error("❌ Transaction history error:", error);
          reject(new Error(error));
          delete window.onGetTransactionHistoryResponse;
          delete window.onGetTransactionHistoryError;
        };

        window.cefMessage?.send('get_transaction_history', []);
      });
    }
  };
}

// overlayPanel methods removed - now using process-per-overlay architecture
