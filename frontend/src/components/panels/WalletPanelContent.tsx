
import { useState } from 'react';
import { TransactionForm } from '../TransactionForm';
import { useBalance } from '../../hooks/useBalance';
import { useAddress } from '../../hooks/useAddress';
// Removed useWallet import - private keys handled by Go daemon
import type { TransactionResponse } from '../../types/transaction';
import '../../components/TransactionComponents.css';
import '../../components/WalletPanel.css';

const WalletPanel = () => {
  const { balance, usdValue, isLoading: balanceLoading, refreshBalance } = useBalance();
  const { currentAddress, isGenerating, generateAndCopy } = useAddress();

  const [showSendForm, setShowSendForm] = useState(false);
  const [transactionResult, setTransactionResult] = useState<TransactionResponse | null>(null);
  const [showReceiveAddress, setShowReceiveAddress] = useState(false);
  const [addressCopiedMessage, setAddressCopiedMessage] = useState<string | null>(null);

  // No wallet initialization needed - using hardcoded test address

  const handleSendClick = () => {
    // Clear all other display states first
    setShowReceiveAddress(false);
    setAddressCopiedMessage(null);
    setTransactionResult(null);

    // Toggle send form
    setShowSendForm(!showSendForm);
  };

  const handleReceiveClick = async () => {
    console.log('🔄 Receive button clicked');

    // Clear all other display states first
    setShowSendForm(false);
    setTransactionResult(null);

    try {
      // Generate address from identity
      const addressData = await generateAndCopy();
      console.log('✅ Address generated and copied:', addressData);

      setShowReceiveAddress(true);
      setAddressCopiedMessage(`Address copied to clipboard: ${addressData.substring(0, 10)}...`);
      console.log('✅ Message set:', `Address copied to clipboard: ${addressData.substring(0, 10)}...`);

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

  const handleSendSubmit = (result: TransactionResponse) => {
    // Clear all other states first
    setShowReceiveAddress(false);
    setAddressCopiedMessage(null);
    setTransactionResult(null);

    // Set the transaction result and close the form
    setTransactionResult(result);
    setShowSendForm(false);
    // Refresh balance after successful transaction
    refreshBalance();
  };


  const clearAllStates = () => {
    setShowSendForm(false);
    setShowReceiveAddress(false);
    setAddressCopiedMessage(null);
    setTransactionResult(null);
  };

  return (
    <div className="wallet-panel-container">
      {/* Balance Display */}
      <div className="balance-display">
          <div className="balance-header">
            <h2>Total Balance</h2>
            <button
              className="refresh-button"
              onClick={refreshBalance}
              disabled={balanceLoading}
            >
              {balanceLoading ? '⏳' : '🔄'} Refresh
            </button>
          </div>
          <div className="balance-content">
            <div className="balance-primary">
              <span className="balance-amount">
                {balanceLoading ? '...' : (balance / 100000000).toFixed(8)}
              </span>
              <span className="balance-currency">BSV</span>
            </div>
            <span className="balance-separator">|</span>
            <div className="balance-secondary">
              <span className="balance-usd">
                ${balanceLoading ? '...' : usdValue.toFixed(2)} USD
              </span>
            </div>
          </div>
          {balanceLoading && (
            <div className="balance-loading">
              <div className="loading-spinner"></div>
              Fetching balance from blockchain...
            </div>
          )}
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

        {/* Navigation Grid */}
        <div className="navigation-grid">
          <button className="nav-grid-button" onClick={clearAllStates}>Certificates</button>
          <button className="nav-grid-button" onClick={clearAllStates}>History</button>
          <button className="nav-grid-button" onClick={clearAllStates}>Settings</button>
          <button className="nav-grid-button" onClick={clearAllStates}>Tokens</button>
          <button className="nav-grid-button" onClick={clearAllStates}>Baskets</button>
          <button className="nav-grid-button" onClick={clearAllStates}>Exchange</button>
        </div>

        {/* Dynamic Content Area */}
        <div className="dynamic-content-area">
          {showSendForm && (
            <div className="send-form-container">
              <TransactionForm
                onTransactionCreated={handleSendSubmit}
                balance={balance}
              />
            </div>
          )}

          {showReceiveAddress && (
            <div className="receive-address-container">
              <h3>Receive Bitcoin SV</h3>
              <p>Address copied to clipboard!</p>
              <div className="address-display">
                <code>{currentAddress || 'Generating...'}</code>
                <button
                  className="copy-button"
                  onClick={() => navigator.clipboard.writeText(currentAddress || '')}
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


          {/* Success Modal */}
          {transactionResult && (
            <div className="success-message">
              <h3>✅ Transaction Sent!</h3>
              <div className="transaction-details">
                <p><strong>TxID:</strong> {transactionResult.txid}</p>
                <p><strong>Status:</strong> {transactionResult.message}</p>
              </div>
              {transactionResult.whatsOnChainUrl && (
                <div className="whatsonchain-container">
                  <a
                    href={transactionResult.whatsOnChainUrl}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="whatsonchain-link"
                    style={{ color: 'var(--wallet-text-light)', textDecoration: 'underline' }}
                  >
                    View on WhatsOnChain
                  </a>
                  <button
                    onClick={() => {
                      if (transactionResult.whatsOnChainUrl) {
                        navigator.clipboard.writeText(transactionResult.whatsOnChainUrl);
                        // You could add a temporary "Copied!" message here if desired
                      }
                    }}
                    className="copy-link-button"
                    style={{
                      marginLeft: '10px',
                      padding: '4px 8px',
                      backgroundColor: 'var(--wallet-gold-accent)',
                      color: 'var(--wallet-text-dark)',
                      border: '1px solid var(--wallet-text-light)',
                      borderRadius: '4px',
                      cursor: 'pointer',
                      fontSize: '12px'
                    }}
                  >
                    Copy Link
                  </button>
                </div>
              )}
              <button onClick={() => setTransactionResult(null)} className="close-button">
                Close
              </button>
            </div>
          )}

          {!showSendForm && !showReceiveAddress && !transactionResult && (
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
  );
};

export default WalletPanel;
