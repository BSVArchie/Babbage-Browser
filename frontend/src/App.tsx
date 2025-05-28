import { useEffect, useState } from 'react';
import { Routes, Route, Navigate, useNavigate } from 'react-router-dom';
import Welcome from './pages/Welcome';
import Dashboard from './pages/Dashboard';
import SidebarLayout from './components/SidebarLayout';

const App = () => {
  const [walletExists, setWalletExists] = useState(false);
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate();

  useEffect(() => {
    // Simulate wallet check (could become a file or localStorage check)
    const exists = localStorage.getItem('wallet_exists') === 'true';
    setWalletExists(exists);
    setLoading(false);
    if (exists) navigate('/dashboard');
  }, []);

  if (loading) return <div>Loading...</div>;

  return (
    <Routes>
      <Route path="/" element={<Navigate to={walletExists ? "/dashboard" : "/welcome"} />} />
      <Route path="/welcome" element={<Welcome />} />
      <Route path="/dashboard" element={<SidebarLayout><Dashboard /></SidebarLayout>} />
    </Routes>
  );
};

export default App;
