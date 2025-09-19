import React, { useState, useEffect } from 'react';
import BackupModal from '../components/panels/BackupModal';
import type { IdentityResult, IdentityData } from '../types/identity';

const BackupOverlayRoot: React.FC = () => {
  const [showBackupModal, setShowBackupModal] = useState(false);
  const [identity, setIdentity] = useState<IdentityData | null>(null);

  useEffect(() => {
    console.log("💾 BackupOverlayRoot mounted");
    console.log("💾 cefMessage available:", !!window.cefMessage);
    console.log("💾 cefMessage.send available:", !!(window.cefMessage?.send));

    // Test cefMessage immediately
    if (window.cefMessage?.send) {
      console.log("💾 Testing cefMessage.send from backup overlay");
      // Could add a test message here if needed
    } else {
      console.log("❌ cefMessage not available in backup overlay");
    }

    // Get identity data for the backup modal FIRST
    const getIdentity = async () => {
      try {
        console.log("💾 Getting identity for backup modal...");
        const result = await window.bitcoinBrowser.identity.get();
        console.log("💾 Identity received:", result);

        if (result) {
          setIdentity(result as IdentityData);
        } else {
          // Identity doesn't exist yet - the backup modal will handle creating it
          console.log("💾 No identity found, backup modal will create new one");
          setIdentity({
            address: "Creating...",
            publicKey: "Creating...",
            privateKey: "Creating..."
          } as IdentityData);
        }

        // Only show the modal AFTER we have identity data
        console.log("💾 Identity loaded, now setting showBackupModal to true");
        setShowBackupModal(true);

        // Force CEF to repaint after a short delay
        setTimeout(() => {
          console.log("💾 Forcing CEF repaint...");
          window.cefMessage?.send('force_repaint', []);
        }, 100);

      } catch (err) {
        console.error("💾 Error getting identity:", err);
        // If there's an error, assume identity doesn't exist
        setIdentity({
          address: "Creating...",
          publicKey: "Creating...",
          privateKey: "Creating..."
        } as IdentityData);

        // Show modal even with error state
        console.log("💾 Error state, setting showBackupModal to true");
        setShowBackupModal(true);
      }
    };

    getIdentity();
  }, []);

  console.log("💾 BackupOverlayRoot render - showBackupModal:", showBackupModal, "identity:", identity);

  return (
    <>
      {identity && (
        <BackupModal
          open={showBackupModal}
          onClose={() => {
            console.log("💾 Backup modal closing");
            setShowBackupModal(false);
            // Use the new process-per-overlay close method
            console.log("💾 Calling overlay_close message for backup overlay");
            window.cefMessage?.send('overlay_close', []);
          }}
          identity={identity}
        />
      )}
    </>
  );
};

export default BackupOverlayRoot;
