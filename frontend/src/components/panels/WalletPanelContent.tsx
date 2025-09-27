
import { useState } from 'react';
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
  const { isLoading: transactionLoading, error: transactionError, sendTransaction } = useTransaction();
  const { currentAddress, isGenerating, generateAndCopy } = useAddress();

  const [showSendForm, setShowSendForm] = useState(false);
  const [showConfirmation, setShowConfirmation] = useState(false);
  const [transactionData, setTransactionData] = useState<any>(null);
  const [transactionResult, setTransactionResult] = useState<TransactionResponse | null>(null);
  const [showReceiveAddress, setShowReceiveAddress] = useState(false);
  const [addressCopiedMessage, setAddressCopiedMessage] = useState<string | null>(null);

  // No wallet initialization needed - using hardcoded test address

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

  const handleSendSubmit = (data: any) => {
    setTransactionData(data);
    setShowConfirmation(true);
  };

  const handleConfirmSend = async () => {
    if (!transactionData) return;

    try {
      const result = await sendTransaction(transactionData);
      setTransactionResult(result);
      setShowConfirmation(false);
      setShowSendForm(false);
      // Refresh balance after successful transaction
      refreshBalance();
    } catch (error) {
      console.error('Transaction failed:', error);
    }
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

            {/* Confirmation Modal */}
            {showConfirmation && transactionData && (
              <div className="confirmation-modal">
                <div className="modal-content">
                  <h3>Confirm Transaction</h3>
                  <div className="transaction-details">
                    <p><strong>To:</strong> {transactionData.recipient}</p>
                    <p><strong>Amount:</strong> {transactionData.amount} satoshis</p>
                    <p><strong>Fee Rate:</strong> {transactionData.feeRate} sat/byte</p>
                  </div>
                  <div className="modal-buttons">
                    <button onClick={handleConfirmSend} className="confirm-button">
                      Confirm Send
                    </button>
                    <button onClick={() => setShowConfirmation(false)} className="cancel-button">
                      Cancel
                    </button>
                  </div>
                </div>
              </div>
            )}

            {/* Success Modal */}
            {transactionResult && (
              <div className="success-modal">
                <div className="modal-content">
                  <h3>✅ Transaction Sent!</h3>
                  <div className="transaction-details">
                    <p><strong>TxID:</strong> {transactionResult.txid}</p>
                    <p><strong>Status:</strong> {transactionResult.message}</p>
                  </div>
                  {transactionResult.whatsOnChainUrl && (
                    <a
                      href={transactionResult.whatsOnChainUrl}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="whatsonchain-link"
                    >
                      View on WhatsOnChain
                    </a>
                  )}
                  <button onClick={() => setTransactionResult(null)} className="close-button">
                    Close
                  </button>
                </div>
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
