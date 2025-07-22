import React, { useState, useEffect } from 'react';
import WalletPanelLayout from '../components/panels/WalletPanelLayout';


const OverlayRoot: React.FC = () => {
  const [walletOpen, setWalletOpen] = useState(false);

  useEffect(() => {
    window.triggerPanel = (panelName: string) => {
        console.log("🔁 Panel trigger received:", panelName);
        if (panelName === 'wallet') {
        setWalletOpen(true);
        }
        // Add other panels later like settings, modal, etc.
    };

    return () => {
        delete window.triggerPanel;
    };
  }, []);

  useEffect(() => {
    if (!walletOpen) {
        window.cefMessage?.send?.('overlay_hide', []);
        console.log("🧹 Overlay HWND requested to hide.");
    }
    }, [walletOpen]);

  return (
    <>
      <WalletPanelLayout
        open={walletOpen}
        onClose={() => setWalletOpen(false)}
      />
    </>
  );
};

export default OverlayRoot;
