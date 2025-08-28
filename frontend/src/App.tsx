
import { useEffect, useState } from 'react';
import { Routes, Route } from 'react-router-dom';
import OverlayRoot from './pages/OverlayRoot';
import MainBrowserView from './pages/MainBrowserView';
import type { IdentityResult } from './types/identity';

const App = () => {
  const [walletExists, setWalletExists] = useState(false);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const loadIdentity = async () => {
      console.log("🚀 useEffect started on", window.location.href);

      for (let i = 0; i < 40; i++) {
        if (typeof window.bitcoinBrowser.identity?.get === 'function') break;
        await new Promise((r) => setTimeout(r, 50));
      }

      if (typeof window.bitcoinBrowser.identity?.get !== 'function') {
        console.warn("⚠️ identity.get is not a function.");
        return;
      }

      try {
        const result: IdentityResult = await window.bitcoinBrowser.identity.get();

        if (result.backedUp === true) {
          console.log("✅ Wallet is backed up.");
          setWalletExists(true);
          setLoading(false);
          return;
        }

        console.log("🧾 Identity JSON:", result);
        setWalletExists(true);
        setLoading(false);

        // Trigger overlay to show backup modal
        if (window.bitcoinBrowser?.overlay?.show) {
          window.bitcoinBrowser.overlay.show();
          // The OverlayRoot will handle showing the backup modal
        }

      } catch (err) {
        console.error("💥 Error in identity.get():", err);
      }
    };

    loadIdentity();
  }, []);

  return (
    <>
      <Routes>
        <Route path="/" element={walletExists ? <MainBrowserView /> : <OverlayRoot />} />
        <Route path="/overlay" element={<OverlayRoot />} />
      </Routes>
    </>
  );
};

export default App;
