
import { useState, useEffect } from 'react';
import { BalanceDisplay } from '../BalanceDisplay';
import { TransactionForm } from '../TransactionForm';
import { useBalance } from '../../hooks/useBalance';
import { useTransaction } from '../../hooks/useTransaction';
import { useAddress } from '../../hooks/useAddress';
// Removed useWallet import - private keys handled by Go daemon
import type { TransactionResponse } from '../../types/transaction';
import '../../components/TransactionComponents.css';

const WalletPanel = () => {
  const { balance, usdValue, isLoading: balanceLoading, refreshBalance } = useBalance();
  const { isLoading: transactionLoading, error: transactionError } = useTransaction();
  const { currentAddress, isGenerating, generateAndCopy } = useAddress();

  const [showSendForm, setShowSendForm] = useState(false);
  const [transactionResult, setTransactionResult] = useState<TransactionResponse | null>(null);
  const [showReceiveAddress, setShowReceiveAddress] = useState(false);
  const [addressCopiedMessage, setAddressCopiedMessage] = useState<string | null>(null);

  // Refresh balance when wallet panel opens
  useEffect(() => {
    refreshBalance();
  }, [refreshBalance]);

  const handleSendClick = () => {
    setShowSendForm(!showSendForm);
  };

  const handleReceiveClick = async () => {
    console.log('🔄 Receive button clicked');
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

  const handleSendSubmit = async (result: TransactionResponse) => {
    // Transaction was already sent, just display the result
    setTransactionResult(result);
    setShowSendForm(false);
    // Refresh balance after successful transaction
    refreshBalance();
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
                  onTransactionCreated={handleSendSubmit}
                  balance={balance}
                  isLoading={transactionLoading}
                  error={transactionError}
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



            {/* Transaction Confirmation Display */}
            {transactionResult && (
              <div className="transaction-confirmation">
                <h3>✅ Transaction Sent Successfully!</h3>
                <div className="transaction-details">
                  <p><strong>Transaction ID:</strong> {transactionResult.txid}</p>
                  <p><strong>Status:</strong> {transactionResult.message}</p>
                </div>
                {transactionResult.whatsOnChainUrl && (
                  <div className="transaction-actions">
                    <a
                      href={transactionResult.whatsOnChainUrl}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="whatsonchain-link"
                    >
                      View on WhatsOnChain
                    </a>
                    <button
                      onClick={() => navigator.clipboard.writeText(transactionResult.whatsOnChainUrl || '')}
                      className="copy-link-button"
                    >
                      Copy Link
                    </button>
                  </div>
                )}
                <button
                  onClick={() => setTransactionResult(null)}
                  className="close-confirmation-button"
                >
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
      </div>
    </div>
  );
};

export default WalletPanel;
