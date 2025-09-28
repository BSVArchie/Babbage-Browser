import React, { useState, useCallback } from 'react';
import { useTransaction } from '../hooks/useTransaction';
import type { TransactionData, TransactionResponse } from '../types/transaction';

interface TransactionFormProps {
  onTransactionCreated: (result: TransactionResponse) => void;
  balance: number;
  isLoading?: boolean;
  error?: string | null;
}

export const TransactionForm: React.FC<TransactionFormProps> = ({
  onTransactionCreated,
  balance,
  isLoading = false,
  error
}) => {
  const { sendTransaction } = useTransaction();
  // Private key is handled by the Go daemon using the identity file
  const [formData, setFormData] = useState<TransactionData>({
    recipient: '',
    amount: '',
    feeRate: '5',
    memo: ''
  });
  const [errors, setErrors] = useState<Partial<TransactionData>>({});
  const [isSubmitting, setIsSubmitting] = useState(false);

  const validateForm = useCallback((): boolean => {
    const newErrors: Partial<TransactionData> = {};

    // Validate recipient address
    if (!formData.recipient.trim()) {
      newErrors.recipient = 'Recipient address is required';
    } else if (!/^[13][a-km-zA-HJ-NP-Z1-9]{25,34}$/.test(formData.recipient)) {
      newErrors.recipient = 'Invalid Bitcoin address format';
    }

    // Validate amount
    const amount = parseFloat(formData.amount);
    if (!formData.amount.trim()) {
      newErrors.amount = 'Amount is required';
    } else if (isNaN(amount) || amount <= 0) {
      newErrors.amount = 'Amount must be a positive number';
    } else if (amount < 0.00000546) { // Minimum 546 satoshis
      newErrors.amount = 'Amount must be at least 546 satoshis (0.00000546 BSV)';
    } else if (amount * 100000000 > balance) { // balance is already in satoshis
      newErrors.amount = 'Insufficient balance';
    }

    // Validate fee rate
    const feeRate = parseInt(formData.feeRate);
    if (isNaN(feeRate) || feeRate < 1 || feeRate > 1000) {
      newErrors.feeRate = 'Fee rate must be between 1 and 1000 satoshis per byte';
    }

    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  }, [formData, balance]);

  const handleInputChange = useCallback((field: keyof TransactionData, value: string) => {
    setFormData(prev => ({ ...prev, [field]: value }));

    // Clear error for this field when user starts typing
    if (errors[field]) {
      setErrors(prev => ({ ...prev, [field]: undefined }));
    }
  }, [errors]);

  const handleSubmit = useCallback(async (e: React.FormEvent) => {
    e.preventDefault();

    if (!validateForm()) {
      console.log('❌ Form validation failed, not submitting');
      return;
    }

    setIsSubmitting(true);
    try {
      console.log('🚀 Form: Starting transaction send with:', formData);

      // Send transaction (create + sign + broadcast in one call)
      console.log('📝 Sending transaction...');
      const result = await sendTransaction(formData);
      console.log('✅ Transaction sent successfully:', result);

      console.log('🎉 Transaction send successful:', result);

      // Call callback with result first (this will close the form)
      try {
        onTransactionCreated(result);
        console.log('🔄 Form: onTransactionCreated called with result');
      } catch (err) {
        console.error('❌ Form: Callback failed:', err);
      }
    } catch (err) {
      console.error('❌ Form: Transaction flow failed:', err);
    } finally {
      console.log('🏁 Form: Setting isSubmitting to false');
      setIsSubmitting(false);
    }
  }, [formData, validateForm, sendTransaction, onTransactionCreated]);

  const formatBalance = useCallback((satoshis: number): string => {
    const bsv = satoshis / 100000000;
    return bsv.toFixed(8);
  }, []);

  return (
    <div className="transaction-form">
      <div className="form-header">
        <h3>Send Bitcoin SV</h3>
        <div className="balance-info">
          <span className="balance-label">Available Balance:</span>
          <span className="balance-amount">{formatBalance(balance)} BSV</span>
        </div>
      </div>

      {error && (
        <div className="error-message">
          <span className="error-icon">⚠️</span>
          {error}
        </div>
      )}

      <form onSubmit={handleSubmit} className="transaction-form-content">
        <div className="form-group">
          <label htmlFor="recipient">Recipient Address</label>
          <input
            id="recipient"
            type="text"
            value={formData.recipient}
            onChange={(e) => handleInputChange('recipient', e.target.value)}
            placeholder="Enter Bitcoin SV address"
            className={errors.recipient ? 'error' : ''}
            disabled={isSubmitting || isLoading}
          />
          {errors.recipient && <span className="field-error">{errors.recipient}</span>}
        </div>

        <div className="form-group">
          <label htmlFor="amount">Amount (BSV)</label>
          <div className="amount-input-container">
            <input
              id="amount"
              type="number"
              step="0.00000001"
              min="0.00000546"
              max={formatBalance(balance)}
              value={formData.amount}
              onChange={(e) => handleInputChange('amount', e.target.value)}
              placeholder="0.00000000"
              className={errors.amount ? 'error' : ''}
              disabled={isSubmitting || isLoading}
            />
            <button
              type="button"
              className="max-button"
              onClick={() => handleInputChange('amount', formatBalance(balance))}
              disabled={isSubmitting || isLoading}
            >
              MAX
            </button>
          </div>
          {errors.amount && <span className="field-error">{errors.amount}</span>}
        </div>

        <div className="form-group">
          <label htmlFor="feeRate">Fee Rate (sat/byte)</label>
          <div className="fee-rate-container">
            <input
              id="feeRate"
              type="number"
              min="1"
              max="1000"
              value={formData.feeRate}
              onChange={(e) => handleInputChange('feeRate', e.target.value)}
              className={errors.feeRate ? 'error' : ''}
              disabled={isSubmitting || isLoading}
            />
            <div className="fee-presets">
              <button
                type="button"
                onClick={() => handleInputChange('feeRate', '1')}
                disabled={isSubmitting || isLoading}
                className={formData.feeRate === '1' ? 'active' : ''}
              >
                Low
              </button>
              <button
                type="button"
                onClick={() => handleInputChange('feeRate', '5')}
                disabled={isSubmitting || isLoading}
                className={formData.feeRate === '5' ? 'active' : ''}
              >
                Medium
              </button>
              <button
                type="button"
                onClick={() => handleInputChange('feeRate', '10')}
                disabled={isSubmitting || isLoading}
                className={formData.feeRate === '10' ? 'active' : ''}
              >
                High
              </button>
            </div>
          </div>
          {errors.feeRate && <span className="field-error">{errors.feeRate}</span>}
        </div>

        <div className="form-group">
          <label htmlFor="memo">Memo (Optional)</label>
          <input
            id="memo"
            type="text"
            value={formData.memo}
            onChange={(e) => handleInputChange('memo', e.target.value)}
            placeholder="Add a note to this transaction"
            disabled={isSubmitting || isLoading}
          />
        </div>

        <button
          type="submit"
          className="submit-button"
          disabled={isSubmitting || isLoading || Object.keys(errors).length > 0}
        >
          {isSubmitting ? 'Creating Transaction...' : 'Send Transaction'}
        </button>
      </form>
    </div>
  );
};
