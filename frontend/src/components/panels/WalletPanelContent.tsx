
import React from 'react';
import AddressManager from '../AddressManager';

const WalletPanel = () => {
  return (
    <div className="p-4">
      <h2 className="text-lg font-semibold mb-4">Wallet Dashboard</h2>
      <p className="mb-4">Wallet is initialized. Manage your addresses and transactions here.</p>

      {/* Address Management */}
      <AddressManager />
    </div>
  );
};

export default WalletPanel;
