import { useNavigate } from 'react-router-dom';

const Welcome = () => {
  const navigate = useNavigate();

  const handleSetup = () => {
    localStorage.setItem('wallet_exists', 'true');
    navigate('/dashboard');
  };

  return (
    <div className="p-4">
      <h1 className="text-xl font-bold">Welcome to Bitcoin Browser</h1>
      <p>This is the initial wallet setup screen.</p>
      <button onClick={handleSetup} className="mt-4 px-4 py-2 bg-blue-500 text-white rounded">
        Set Up Wallet
      </button>
    </div>
  );
};

export default Welcome;
