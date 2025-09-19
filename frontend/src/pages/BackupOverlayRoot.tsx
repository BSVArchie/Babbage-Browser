import React, { useState, useEffect } from 'react';
import BackupModal from '../components/panels/BackupModal';
import type { IdentityResult, IdentityData } from '../types/identity';

const BackupOverlayRoot: React.FC = () => {
  const [showBackupModal, setShowBackupModal] = useState(false);
  const [identity, setIdentity] = useState<IdentityData | null>(null);

  useEffect(() => {
    console.log("💾 BackupOverlayRoot mounted");

    const createIdentity = async () => {
      console.log("💾 Creating/getting identity for backup modal...");

      // Wait for bitcoinBrowser API to be ready
      await new Promise((resolve) => {
        if (window.bitcoinBrowser) {
          resolve(true);
        } else {
          window.addEventListener('bitcoinBrowserReady', resolve, { once: true });
          // Fallback timeout
          setTimeout(resolve, 1000);
        }
      });

      console.log("💾 bitcoinBrowser API ready, proceeding with identity creation");

      // Set up response listener
      const handleResponse = (event: any) => {
        if (event.detail.message === 'create_identity_response') {
          try {
            const response = JSON.parse(event.detail.args[0]);
            console.log("💾 Create identity response:", response);

            if (response.success && response.identity) {
              setIdentity(response.identity as IdentityData);
              console.log("💾 Identity loaded, showing backup modal");
              setShowBackupModal(true);

              // Force CEF to repaint after modal shows
              setTimeout(() => {
                console.log("💾 Forcing CEF repaint...");
                window.cefMessage?.send('force_repaint', []);
              }, 800);
            } else {
              console.error("💾 Failed to create/get identity:", response.error);
              // Show modal with error state
              setIdentity({
                address: "Error creating identity",
                publicKey: response.error || "Unknown error",
                privateKey: "Please try again"
              } as IdentityData);
              setShowBackupModal(true);
            }
          } catch (error) {
            console.error("💾 Error parsing create identity response:", error);
            // Show modal with error state
            setIdentity({
              address: "Error parsing response",
              publicKey: "Unknown error",
              privateKey: "Please try again"
            } as IdentityData);
            setShowBackupModal(true);
          }

          // Remove listener
          window.removeEventListener('cefMessageResponse', handleResponse);
        }
      };

      window.addEventListener('cefMessageResponse', handleResponse);

      // Send create identity request
      if (window.cefMessage?.send) {
        window.cefMessage.send('create_identity', []);
      } else {
        console.error("💾 cefMessage not available");
        setIdentity({
          address: "cefMessage not available",
          publicKey: "Backend not ready",
          privateKey: "Please restart the app"
        } as IdentityData);
        setShowBackupModal(true);
      }

      // Cleanup listener after timeout
      setTimeout(() => {
        window.removeEventListener('cefMessageResponse', handleResponse);
      }, 10000);
    };

    createIdentity();
  }, []);

  console.log("💾 BackupOverlayRoot render - showBackupModal:", showBackupModal, "identity:", identity);

  return (
    <>
      {identity ? (
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
      ) : (
        <div style={{
          position: 'fixed',
          top: 0,
          left: 0,
          width: '100%',
          height: '100%',
          backgroundColor: 'rgba(0, 0, 0, 0.8)',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          color: 'white',
          fontSize: '24px',
          fontFamily: 'Arial, sans-serif'
        }}>
          <div style={{ textAlign: 'center' }}>
            <div style={{ marginBottom: '20px' }}>🔄</div>
            <div>Creating Identity...</div>
            <div style={{ fontSize: '16px', marginTop: '10px', opacity: 0.7 }}>
              Please wait while we set up your wallet
            </div>
          </div>
        </div>
      )}
    </>
  );
};

export default BackupOverlayRoot;
