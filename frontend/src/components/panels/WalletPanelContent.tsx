
import { useState } from 'react';
import { BalanceDisplay } from '../BalanceDisplay';
import { TransactionForm } from '../TransactionForm';
import { useBalance } from '../../hooks/useBalance';
import { useTransaction } from '../../hooks/useTransaction';
import { useAddress } from '../../hooks/useAddress';
import type { TransactionResponse } from '../../types/transaction';
import '../../components/TransactionComponents.css';

const WalletPanel = () => {
  const { balance, usdValue, isLoading: balanceLoading, refreshBalance } = useBalance();
  const { isLoading: transactionLoading, error: transactionError } = useTransaction();
  const { currentAddress, isGenerating, generateAndCopy } = useAddress();

  const [showSendForm, setShowSendForm] = useState(false);
  const [lastTransaction, setLastTransaction] = useState<TransactionResponse | null>(null);
  const [showReceiveAddress, setShowReceiveAddress] = useState(false);
  const [addressCopiedMessage, setAddressCopiedMessage] = useState<string | null>(null);

  const handleSendClick = () => {
    setShowSendForm(!showSendForm);
  };

  const handleReceiveClick = async () => {
    console.log('🔄 Receive button clicked');
    try {
      console.log('🔄 Calling generateAndCopy...');
      const address = await generateAndCopy();
      console.log('✅ Address generated and copied:', address);

      setShowReceiveAddress(true);
      setAddressCopiedMessage(`Address copied to clipboard: ${address.substring(0, 10)}...`);
      console.log('✅ Message set:', `Address copied to clipboard: ${address.substring(0, 10)}...`);

      // Clear the message after 3 seconds
      setTimeout(() => {
        console.log('🔄 Clearing address copied message');
        setAddressCopiedMessage(null);
      }, 3000);
    } catch (error) {
      console.error('❌ Failed to generate address:', error);
      setAddressCopiedMessage(`Error: ${error instanceof Error ? error.message : 'Unknown error'}`);
    }
  };

  const handleTransactionCreated = (result: TransactionResponse) => {
    setLastTransaction(result);
    setShowSendForm(false); // Hide form after successful transaction
    // Refresh balance after transaction
    refreshBalance().catch(err => console.error('Balance refresh failed:', err));
  };

  return (
    <div className="wallet-panel">
      {/* Header */}
      <div className="wallet-header">
        <h1>Bitcoin SV Wallet</h1>
      </div>

      {/* Main Content */}
      <div className="wallet-main-content">
        {/* Left Panel - Navigation (Future) */}
        <div className="wallet-left-panel">
          <div className="nav-item">Exchange</div>
          <div className="nav-item">Certificates (Identity)</div>
          <div className="nav-item">Tokens/Baskets</div>
          <div className="nav-item">History</div>
          <div className="nav-item">Settings</div>
        </div>

        {/* Right Panel - Wallet Functionality */}
        <div className="wallet-right-panel">
          {/* Top Section */}
          <div className="wallet-top-section">
            {/* Logo Area */}
            <div className="wallet-logo">
              <div className="logo-placeholder">Cool logo or something</div>
            </div>

            {/* Balance Display */}
            <div className="wallet-balance">
              <BalanceDisplay
                balance={balance}
                usdValue={usdValue}
                isLoading={balanceLoading}
                onRefresh={refreshBalance}
              />
            </div>
          </div>

          {/* Action Buttons */}
          <div className="wallet-actions">
            <button
              className="wallet-button receive-button"
              onClick={handleReceiveClick}
              disabled={isGenerating}
            >
              {isGenerating ? 'Generating...' : 'Receive'}
            </button>
            <button
              className="wallet-button send-button"
              onClick={handleSendClick}
            >
              Send
            </button>
          </div>

          {/* Dynamic Content Area */}
          <div className="wallet-content-area">
            {showSendForm && (
              <div className="send-form-container">
                <TransactionForm
                  onTransactionCreated={handleTransactionCreated}
                  balance={balance}
                  isLoading={transactionLoading}
                  error={transactionError}
                />
              </div>
            )}

            {showReceiveAddress && currentAddress && (
              <div className="receive-address-container">
                <h3>Receive Bitcoin SV</h3>
                <p>Address copied to clipboard!</p>
                <div className="address-display">
                  <code>{currentAddress}</code>
                  <button
                    className="copy-button"
                    onClick={() => navigator.clipboard.writeText(currentAddress)}
                  >
                    Copy Again
                  </button>
                </div>
                <button
                  className="close-button"
                  onClick={() => setShowReceiveAddress(false)}
                >
                  Close
                </button>
              </div>
            )}

            {lastTransaction && (
              <div className="success-message">
                <strong>✅ Transaction Created Successfully!</strong>
                <br />
                <strong>Transaction ID:</strong> {lastTransaction.txid}
                <br />
                <strong>Fee:</strong> {lastTransaction.fee} satoshis
                <br />
                <strong>Status:</strong> {lastTransaction.status}
              </div>
            )}

            {!showSendForm && !showReceiveAddress && !lastTransaction && (
              <div className="content-placeholder">
                {addressCopiedMessage ? (
                  <div className="address-copied-message">
                    ✅ {addressCopiedMessage}
                  </div>
                ) : (
                  "Area to render stuff the user clicks on or something"
                )}
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
};

export default WalletPanel;
