import { useState, useCallback, useEffect } from 'react';
// Removed useWallet import - using HD wallet system directly

export const useBalance = () => {
  const [balance, setBalance] = useState(0);
  const [usdValue, setUsdValue] = useState(0);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const fetchBalance = useCallback(async (): Promise<number> => {
    setIsLoading(true);
    setError(null);

    try {
      // Call C++ bridge via window.bitcoinBrowser.wallet
      if (!window.bitcoinBrowser?.wallet) {
        throw new Error('Bitcoin Browser wallet not available');
      }

      // Get total balance across all addresses (no address parameter needed)
      const response = await window.bitcoinBrowser.wallet.getBalance();

      setBalance(response.balance);
      return response.balance;
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : 'Failed to fetch balance';
      setError(errorMessage);
      throw new Error(errorMessage);
    } finally {
      setIsLoading(false);
    }
  }, []);

  const fetchUsdPrice = useCallback(async (): Promise<number> => {
    try {
      console.log('🔍 Fetching BSV price from CryptoCompare API...');

      // Use only CryptoCompare API - no fallbacks
      const response = await fetch('https://min-api.cryptocompare.com/data/price?fsym=BSV&tsyms=USD', {
        method: 'GET',
        headers: {
          'Accept': 'application/json',
          'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
        },
        mode: 'cors'
      });

      if (!response.ok) {
        throw new Error(`CryptoCompare API failed with status: ${response.status}`);
      }

      const data = await response.json();
      console.log('🔍 CryptoCompare response:', data);

      const price = parseFloat(data.USD);
      if (!price || price <= 0) {
        throw new Error('Invalid price data received from CryptoCompare API');
      }

      const usdValue = (balance / 100000000) * price; // Convert satoshis to BSV, then to USD
      setUsdValue(usdValue);

      console.log(`💰 BSV Price: $${price}, Balance: ${balance} satoshis, USD Value: $${usdValue.toFixed(2)}`);
      return usdValue;

    } catch (err) {
      console.error('❌ Failed to fetch BSV price:', err);
      console.error('🔍 This indicates a problem with the CryptoCompare API - investigate network connectivity and API status');
      setUsdValue(0);
      throw new Error(`Price fetch failed: ${err instanceof Error ? err.message : 'Unknown error'}`);
    }
  }, [balance]);


  const refreshBalance = useCallback(async () => {
    await fetchBalance();
    await fetchUsdPrice();
  }, [fetchBalance, fetchUsdPrice]);

  // Auto-refresh balance every 30 seconds - DISABLED FOR DEBUGGING
  // useEffect(() => {
  //   const interval = setInterval(() => {
  //     refreshBalance();
  //   }, 30000);

  //   return () => clearInterval(interval);
  // }, [refreshBalance]);

  // Initial load
  useEffect(() => {
    refreshBalance();
  }, [refreshBalance]);

  return {
    balance,
    usdValue,
    isLoading,
    error,
    fetchBalance,
    fetchUsdPrice,
    refreshBalance
  };
};
