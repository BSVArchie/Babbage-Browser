import { useEffect, useState } from 'react';
import { Routes, Route, Navigate, useNavigate } from 'react-router-dom';
import Welcome from './pages/Welcome';
import Dashboard from './pages/Dashboard';
import SidebarLayout from './components/SidebarLayout';
import BackupModal from './components/BackupModal';

declare global {
  interface Window {
    identity?: {
      get: () => string;
    };
  }
}

const App = () => {
  const [walletExists, setWalletExists] = useState(false);
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate();
  const [showBackupModal, setShowBackupModal] = useState(false);
  const [identity, setIdentity] = useState({ address: '', publicKey: '', privateKey: '' });

  useEffect(() => {
    const exists = localStorage.getItem('wallet_exists') === 'true';
    setWalletExists(exists);
    setLoading(false);

    if (exists) {
      // Simulate identity JSON load
      const storedIdentity = localStorage.getItem('identity');
      if (storedIdentity) {
        setIdentity(JSON.parse(storedIdentity));
      } else {
        // TEMP: This would be replaced with the real identity load from native backend
        setIdentity({
          address: 'mock-address',
          publicKey: 'mock-public-key',
          privateKey: 'mock-private-key'
        });
      }

      // Simulate logic: if they haven’t backed up, prompt them
      const backedUp = localStorage.getItem('identity_backed_up') === 'true';
      if (!backedUp) setShowBackupModal(true);

      navigate('/dashboard');
    }
  }, []);

  useEffect(() => {
    if (window.identity?.get) {
      const result = window.identity.get();  // Should be JSON string
      const json = JSON.parse(result);
      console.log("✅ Identity JSON loaded:", json);
    } else {
      console.warn("⚠️ identity.get() not available");
    }
  }, []);

  if (loading) return <div>Loading...</div>;

  return (
    <>
      <Routes>
        <Route path="/" element={<Navigate to={walletExists ? "/dashboard" : "/welcome"} />} />
        <Route path="/welcome" element={<Welcome />} />
        <Route path="/dashboard" element={<SidebarLayout><Dashboard /></SidebarLayout>} />
      </Routes>

        <BackupModal
        open={showBackupModal}
        onClose={() => {
          setShowBackupModal(false);
          localStorage.setItem('identity_backed_up', 'true'); // Mark as backed up
        }}
        identity={identity}
      />
    </>
  );
};

export default App;
