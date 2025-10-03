import { useEffect } from 'react';
import { Routes, Route } from 'react-router-dom';
import SettingsOverlayRoot from './pages/SettingsOverlayRoot';
import WalletOverlayRoot from './pages/WalletOverlayRoot';
import BackupOverlayRoot from './pages/BackupOverlayRoot';
import MainBrowserView from './pages/MainBrowserView';
import { AuthApprovalModal, TransactionApprovalModal, useBRC100Modals } from './components/BRC100Modals';
import { brc100 } from './bridge/brc100';
// Removed identity types - now using unified wallet system

const App = () => {
  console.log("🔍 App component rendering, pathname:", window.location.pathname);

  // BRC-100 modal management
  const {
    authModal,
    transactionModal,
    showAuthApprovalModal,
    showTransactionApprovalModal,
    handleAuthApprove,
    handleAuthReject,
    handleTransactionApprove,
    handleTransactionReject
  } = useBRC100Modals();

  // Wallet state tracking (currently unused but available for future features)
  // const [walletExists, setWalletExists] = useState(false);

  useEffect(() => {
    console.log("🔍 useEffect started");

    const checkWalletStatus = async () => {
      console.log("🔍 checkWalletStatus started");

      // Wait for all systems to be ready (for overlay browsers)
      if (window.location.pathname !== '/') {
        await new Promise<void>((resolve) => {
          if (window.allSystemsReady) {
            console.log("🔍 All systems already ready");
            resolve();
          } else {
            console.log("🔍 Waiting for allSystemsReady event...");
            window.addEventListener('allSystemsReady', () => {
              console.log("🔍 allSystemsReady event received");
              resolve();
            }, { once: true });
          }
        });
      }

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
      if (window.location.pathname === '/' && window.bitcoinBrowser?.wallet) {
        console.log("🔍 Running wallet status check via API");

        try {
          const walletStatus = await window.bitcoinBrowser.wallet.getStatus();
          console.log("🔍 Wallet status response:", walletStatus);

          if (walletStatus.needsBackup) {
            // Wallet needs backup - create wallet first, then show modal
            console.log("🔍 Wallet needs backup, creating wallet first...");
            try {
              await window.bitcoinBrowser.wallet.create();
              console.log("🔍 Wallet created successfully, showing backup modal");
              window.cefMessage?.send('overlay_show_backup', []);
            } catch (error) {
              console.error("💥 Error creating wallet:", error);
            }
          } else {
            // Wallet is backed up - do nothing
            console.log("🔍 Wallet is backed up, no action needed");
          }
        } catch (error) {
          console.error("💥 Error checking wallet status:", error);
        }

      } else {
        console.log("🔍 Skipping wallet check - path:", window.location.pathname, "wallet API ready:", !!window.bitcoinBrowser?.wallet);
      }
    };

    checkWalletStatus();

    // Initialize BRC-100 API integration
    const initializeBRC100 = async () => {
      try {
        // Check if BRC-100 is available
        const isAvailable = await brc100.isAvailable();
        console.log("🔐 BRC-100 available:", isAvailable);

        if (isAvailable) {
          // Override the modal methods to use our React components
          (brc100 as any).showAuthApprovalModal = showAuthApprovalModal;
          (brc100 as any).showTransactionApprovalModal = showTransactionApprovalModal;

          console.log("🔐 BRC-100 API initialized with React modals");
        }
      } catch (error) {
        console.warn("🔐 BRC-100 initialization failed:", error);
      }
    };

    initializeBRC100();

    // Cleanup function to remove event listeners
    return () => {
      console.log("🧹 App cleanup - removing event listeners");
      // Note: Event listeners are automatically cleaned up when the component unmounts
      // but this ensures we have explicit cleanup logging
    };
  }, [showAuthApprovalModal, showTransactionApprovalModal]);

  return (
    <>
      <Routes>
        {/* <Route path="/" element={walletExists ? <MainBrowserView /> : <OverlayRoot />} /> */}
        <Route path="/" element={<MainBrowserView />} />
        <Route path="/settings" element={<SettingsOverlayRoot />} />
        <Route path="/wallet" element={<WalletOverlayRoot />} />
        <Route path="/backup" element={<BackupOverlayRoot />} />
      </Routes>

      {/* BRC-100 Approval Modals */}
      <AuthApprovalModal
        isOpen={authModal.isOpen}
        request={authModal.request!}
        onApprove={handleAuthApprove}
        onReject={handleAuthReject}
      />

      <TransactionApprovalModal
        isOpen={transactionModal.isOpen}
        transaction={transactionModal.transaction!}
        onApprove={handleTransactionApprove}
        onReject={handleTransactionReject}
      />
    </>
  );
};

export default App;
