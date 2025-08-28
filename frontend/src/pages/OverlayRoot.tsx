import React, { useState, useEffect } from 'react';
import WalletPanelLayout from '../components/panels/WalletPanelLayout';
import BackupModal from '../components/panels/BackupModal';
import type { IdentityResult, IdentityData } from '../types/identity';

const OverlayRoot: React.FC = () => {
  const [walletOpen, setWalletOpen] = useState(false);
  const [showBackupModal, setShowBackupModal] = useState(false);
  const [identity, setIdentity] = useState<IdentityData | null>(null);  // Change to IdentityData

  // Check for identity on mount
  useEffect(() => {
    const checkIdentity = async () => {
      if (window.bitcoinBrowser?.identity?.get) {
        try {
          const result: IdentityResult = await window.bitcoinBrowser.identity.get();
          // Check if it's IdentityData (not BackupCheck)
          if (result && !result.backedUp && 'address' in result) {
            setIdentity(result as IdentityData);  // Type assertion to IdentityData
            setShowBackupModal(true);
          }
        } catch (err) {
          console.error("Error checking identity:", err);
        }
      }
    };

    checkIdentity();
  }, []);

  useEffect(() => {
    window.triggerPanel = (panelName: string) => {
      console.log("🔁 Panel trigger received:", panelName);
      if (panelName === 'wallet') {
        setWalletOpen(true);
      }
    };

    return () => {
      delete window.triggerPanel;
    };
  }, []);

  useEffect(() => {
    if (!walletOpen && !showBackupModal) {
      window.cefMessage?.send?.('overlay_hide', []);
      console.log("🧹 Overlay HWND requested to hide.");
    }
  }, [walletOpen, showBackupModal]);

  return (
    <>
      <WalletPanelLayout
        open={walletOpen}
        onClose={() => setWalletOpen(false)}
      />

      {identity && (
        <BackupModal
          open={showBackupModal}
          onClose={() => {
            setShowBackupModal(false);
            localStorage.setItem('identity_backed_up', 'true');
            // Hide overlay after backup
            window.cefMessage?.send?.('overlay_hide', []);
          }}
          identity={identity}
        />
      )}
    </>
  );
};

export default OverlayRoot;
