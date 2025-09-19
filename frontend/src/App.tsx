import { useEffect, useState } from 'react';
import { Routes, Route } from 'react-router-dom';
import SettingsOverlayRoot from './pages/SettingsOverlayRoot';
import WalletOverlayRoot from './pages/WalletOverlayRoot';
import BackupOverlayRoot from './pages/BackupOverlayRoot';
import MainBrowserView from './pages/MainBrowserView';
import type { IdentityResult } from './types/identity';

const App = () => {
  console.log("🔍 App component rendering, pathname:", window.location.pathname);
  const [identityFileExists, setIdentityFileExists] = useState(false);

  useEffect(() => {
    console.log("🔍 useEffect started");

    const checkIdentityStatus = async () => {
      console.log("🔍 checkIdentityStatus started");

      // Wait for cefMessage to be ready
      for (let i = 0; i < 40; i++) {
        if (window.cefMessage && typeof window.cefMessage.send === 'function') {
          console.log("🔍 Backend ready after", i, "attempts");
          break;
        }
        await new Promise((r) => setTimeout(r, 50));
      }

      console.log("🔍 Backend check complete, cefMessage exists:", typeof window.cefMessage?.send);
      console.log("🔍 Current pathname:", window.location.pathname);

      // Only check on main page
      if (window.location.pathname === '/' && window.cefMessage && typeof window.cefMessage.send === 'function') {
        console.log("🔍 Running identity status check via message");

        // Set up response listener
        const handleResponse = (event: any) => {
          if (event.detail.message === 'identity_status_check_response') {
            try {
              const response = JSON.parse(event.detail.args[0]);
              console.log("🔍 Identity status response:", response);

              if (response.needsBackup) {
                // Identity needs backup - show backup modal
                console.log("🔍 Identity needs backup, showing backup modal");
                window.cefMessage?.send('overlay_show_backup', []);
              } else {
                // Identity is backed up - do nothing
                console.log("🔍 Identity is backed up, no action needed");
              }
            } catch (error) {
              console.error("💥 Error parsing identity status response:", error);
              // On error, assume we need to show backup modal
              console.log("🔍 Error parsing response, showing backup modal as fallback");
              window.cefMessage?.send('overlay_show_backup', []);
            }

            // Remove listener
            window.removeEventListener('cefMessageResponse', handleResponse);
          }
        };

        window.addEventListener('cefMessageResponse', handleResponse);

        // Send request
        window.cefMessage.send('identity_status_check', []);

        // Cleanup listener after timeout
        setTimeout(() => {
          window.removeEventListener('cefMessageResponse', handleResponse);
        }, 5000);

      } else {
        console.log("🔍 Skipping identity check - path:", window.location.pathname, "backend ready:", typeof window.cefMessage?.send === 'function');
      }
    };

    checkIdentityStatus();

    // Cleanup function to remove event listeners
    return () => {
      console.log("🧹 App cleanup - removing event listeners");
      // Note: Event listeners are automatically cleaned up when the component unmounts
      // but this ensures we have explicit cleanup logging
    };
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
