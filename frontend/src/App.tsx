import { useEffect, useState } from 'react';
import { Routes, Route, Navigate, useNavigate } from 'react-router-dom';
import Welcome from './pages/Welcome';
import BackupModal from './components/panels/BackupModal';
import MainBrowserView from './pages/MainBrowserView';

interface IdentityObject {
    publicKey: string;
    privateKey: string;
    address: string;
    backedUp: boolean;
  }

const App = () => {
  const [walletExists, setWalletExists] = useState(false);
  const [loading, setLoading] = useState(true);
  // const navigate = useNavigate();
  const [showBackupModal, setShowBackupModal] = useState(false);
  const [identity, setIdentity] = useState<IdentityObject | null>(null);

  useEffect(() => {
    const loadIdentity = async () => {

      console.log("window.identity", window.bitcoinBrowser.identity);
      typeof window.bitcoinBrowser.identity?.get;

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
        const result = await window.bitcoinBrowser.identity.get();

        if (result.backedUp === true) {
          console.log("✅ Wallet is backed up.");
          setWalletExists(true);
          setLoading(false);
          return;
        }

        console.log("🧾 Identity JSON:", result);
        setIdentity(result); // TS now knows result is full IdentityData
        setWalletExists(true);
        setShowBackupModal(true);
        setLoading(false);

      } catch (err) {
        console.error("💥 Error in identity.get():", err);
      }
    };

    loadIdentity();
  }, []);

  return (
    <>
      <Routes>
        <Route path="/" element={walletExists ? <MainBrowserView /> : <Welcome />} />
        <Route path="/welcome" element={<Welcome />} />
      </Routes>

        {identity && (
          <BackupModal
            open={showBackupModal}
            onClose={() => {
              setShowBackupModal(false);
              localStorage.setItem('identity_backed_up', 'true'); // Mark as backed up
            }}
            identity={identity}
          />
        )}
    </>
  );
};

export default App;
