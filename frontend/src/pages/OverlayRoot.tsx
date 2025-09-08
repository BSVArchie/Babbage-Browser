import React, { useState, useEffect } from 'react';
import WalletPanelLayout from '../components/panels/WalletPanelLayout';
import BackupModal from '../components/panels/BackupModal';
import type { IdentityResult, IdentityData } from '../types/identity';

const OverlayRoot: React.FC = () => {
  const [walletOpen, setWalletOpen] = useState(false);
  const [showBackupModal, setShowBackupModal] = useState(false);
  const [identity, setIdentity] = useState<IdentityData | null>(null);  // Change to IdentityData

  useEffect(() => {
    window.triggerPanel = (panelName: string) => {
      console.log("🔁 Panel trigger received:", panelName);
      if (panelName === 'wallet') {
        setWalletOpen(true);
      } else if (panelName === 'backup') {
        console.log("🔁 Backup panel triggered - getting identity...");
        const getIdentity = async () => {
          try {
            const result = await window.bitcoinBrowser.identity.get();
            console.log("🔁 Identity received:", result);
            setIdentity(result as IdentityData);
            setShowBackupModal(true);
            console.log("�� Modal state set to true");
          } catch (err) {
            console.error("🔁 Error getting identity:", err);
          }
        };
        getIdentity();
      }
    };
  }, []);

  return (
    <>
      <WalletPanelLayout
        open={walletOpen}
        onClose={() => {
          console.log("🔍 Wallet closing - before:", { walletOpen, showBackupModal });
          setWalletOpen(false);
          console.log("🔍 Wallet closing - after:", { walletOpen: false, showBackupModal });
          window.cefMessage?.send?.('overlay_hide', []);
        }}
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
