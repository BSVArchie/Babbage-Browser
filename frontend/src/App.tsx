
import { useEffect, useState } from 'react';
import { Routes, Route } from 'react-router-dom';
import OverlayRoot from './pages/OverlayRoot';
import MainBrowserView from './pages/MainBrowserView';
import type { IdentityResult } from './types/identity';

const App = () => {
  console.log("🔍 App component rendering, pathname:", window.location.pathname);
  const [identityFileExists, setIdentityFileExists] = useState(false);


  useEffect(() => {
    console.log("🔍 useEffect started");

    const loadIdentity = async () => {
      console.log("🔍 loadIdentity started");

      // Wait for backend to be ready
      for (let i = 0; i < 40; i++) {
        if (typeof window.bitcoinBrowser?.identity?.get === 'function') {
          console.log("🔍 Backend ready after", i, "attempts");
          break;
        }
        await new Promise((r) => setTimeout(r, 50));
      }

      console.log("🔍 Backend check complete, identity.get exists:", typeof window.bitcoinBrowser?.identity?.get);
      console.log("🔍 Current pathname:", window.location.pathname);

      // Now check if we're in header and backend is ready
      if (window.location.pathname === '/' && typeof window.bitcoinBrowser?.identity?.get === 'function') {
        console.log("🔍 Running identity check");
        try {
          const result: IdentityResult = await window.bitcoinBrowser.identity.get();
          console.log("�� Identity result:", result);
          if (result.backedUp === false) {
            console.log("🔍 Triggering backup modal");
            window.bitcoinBrowser.overlay.show();
            window.bitcoinBrowser.overlay.toggleInput(true);
            window.bitcoinBrowser.overlayPanel.open('backup');
          }
        } catch (err) {
          console.error("💥 Error in identity.get():", err);
        }
      } else {
        console.log("🔍 Skipping identity check - path:", window.location.pathname, "backend ready:", typeof window.bitcoinBrowser?.identity?.get === 'function');
      }
    };

    loadIdentity();
  }, []);

  return (
    <>
      <Routes>
        {/* <Route path="/" element={walletExists ? <MainBrowserView /> : <OverlayRoot />} /> */}
        <Route path="/" element={<MainBrowserView />} />
        <Route path="/overlay" element={<OverlayRoot />} />
      </Routes>
    </>
  );
};

export default App;
