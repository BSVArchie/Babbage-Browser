import { useState, useCallback } from 'react';
// Removed useWallet import - private keys handled by Go daemon
import type { TransactionData, Transaction, TransactionResponse, BroadcastResponse } from '../types/transaction';

export const useTransaction = () => {
  // Wallet address will be obtained from the HD wallet system
  // const [walletAddress, setWalletAddress] = useState<string>(''); // Currently unused
  const [transactions, setTransactions] = useState<Transaction[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const createTransaction = useCallback(async (data: TransactionData): Promise<TransactionResponse> => {
    setIsLoading(true);
    setError(null);

    try {
      // Call C++ bridge via window.bitcoinAPI
      if (!window.bitcoinAPI) {
        throw new Error('Bitcoin API not available');
      }

      // Get current address from HD wallet system
      if (!window.bitcoinBrowser?.wallet) {
        throw new Error('Wallet system not available');
      }

      const currentAddress = await window.bitcoinBrowser.wallet.getCurrentAddress();
      if (!currentAddress?.address) {
        throw new Error('No wallet address available - please create or load a wallet first');
      }

      const response = await window.bitcoinAPI.createTransaction({
        recipientAddress: data.recipient,
        amount: parseInt(data.amount),
        feeRate: parseInt(data.feeRate),
        senderAddress: currentAddress.address
      });

      return response;
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to create transaction';
      setError(errorMessage);
      throw new Error(errorMessage);
    } finally {
      setIsLoading(false);
    }
  }, []);

  const signTransaction = useCallback(async (rawTx: string): Promise<TransactionResponse> => {
    setIsLoading(true);
    setError(null);

    try {
      if (!window.bitcoinAPI) {
        throw new Error('Bitcoin API not available');
      }

      const response = await window.bitcoinAPI.signTransaction({
        rawTx
      });

      return response;
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to sign transaction';
      setError(errorMessage);
      throw new Error(errorMessage);
    } finally {
      setIsLoading(false);
    }
  }, []);

  const broadcastTransaction = useCallback(async (rawTx: string): Promise<BroadcastResponse> => {
    setIsLoading(true);
    setError(null);

    try {
      if (!window.bitcoinAPI) {
        throw new Error('Bitcoin API not available');
      }

      const response = await window.bitcoinAPI.broadcastTransaction({
        rawTx
      });

      return response;
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to broadcast transaction';
      setError(errorMessage);
      throw new Error(errorMessage);
    } finally {
      setIsLoading(false);
    }
  }, []);

  const getTransactionHistory = useCallback(async (): Promise<Transaction[]> => {
    setIsLoading(true);
    setError(null);

    try {
      // TODO: Implement when backend supports transaction history
      // const response = await window.bitcoinAPI.getTransactionHistory();
      // return response;

      // Mock data for now
      const mockTransactions: Transaction[] = [
        {
          txid: 'bd1f1edb79ff832bc83b92fe0a0014b7926d0164f75d02dddd1bdc450df8a9e5',
          status: 'confirmed',
          amount: 1000,
          recipient: '1BvBMSEYstWetqTFn5Au4m4GFg7xJaNVN2',
          timestamp: Date.now() - 3600000, // 1 hour ago
          confirmations: 6,
          fee: 1145,
          memo: 'Test transaction'
        }
      ];

      setTransactions(mockTransactions);
      return mockTransactions;
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to fetch transaction history';
      setError(errorMessage);
      throw new Error(errorMessage);
    } finally {
      setIsLoading(false);
    }
  }, []);

  const clearError = useCallback(() => {
    setError(null);
  }, []);

  return {
    transactions,
    isLoading,
    error,
    createTransaction,
    signTransaction,
    broadcastTransaction,
    getTransactionHistory,
    clearError
  };
};
