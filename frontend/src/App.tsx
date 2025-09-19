
import { useEffect, useState, useRef } from 'react';
import { Routes, Route } from 'react-router-dom';
import SettingsOverlayRoot from './pages/SettingsOverlayRoot';
import WalletOverlayRoot from './pages/WalletOverlayRoot';
import BackupOverlayRoot from './pages/BackupOverlayRoot';
import MainBrowserView from './pages/MainBrowserView';
import type { IdentityResult } from './types/identity';

const App = () => {
  console.log("🔍 App component rendering, pathname:", window.location.pathname);
  const [identityFileExists, setIdentityFileExists] = useState(false);
  const backupModalTriggeredRef = useRef(false);


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
          // Handle the three cases:
          if (!result) {
            // Case 1: Identity file doesn't exist - create it and show backup modal
            console.log("🔍 No identity file found, creating new identity");
            // The identity will be created by the backup modal when it loads
            if (!backupModalTriggeredRef.current) {
              console.log("🔍 Triggering backup modal for new identity");
              backupModalTriggeredRef.current = true;
              window.cefMessage?.send('overlay_show_backup', []);
            }
          } else if (result.backedUp === false) {
            // Case 2: Identity exists but not backed up - show backup modal
            if (!backupModalTriggeredRef.current) {
              console.log("🔍 Identity exists but not backed up, triggering backup modal");
              backupModalTriggeredRef.current = true;
              window.cefMessage?.send('overlay_show_backup', []);
            }
          } else {
            // Case 3: Identity exists and is backed up - do nothing
            console.log("🔍 Identity exists and is backed up, no action needed");
          }
        } catch (err) {
          console.error("💥 Error in identity.get():", err);
          // If there's an error getting identity, assume it doesn't exist and create it
          if (!backupModalTriggeredRef.current) {
            console.log("🔍 Error getting identity, assuming it doesn't exist, triggering backup modal");
            backupModalTriggeredRef.current = true;
            window.cefMessage?.send('overlay_show_backup', []);
          }
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
        <Route path="/settings" element={<SettingsOverlayRoot />} />
        <Route path="/wallet" element={<WalletOverlayRoot />} />
        <Route path="/backup" element={<BackupOverlayRoot />} />
      </Routes>
    </>
  );
};

export default App;
