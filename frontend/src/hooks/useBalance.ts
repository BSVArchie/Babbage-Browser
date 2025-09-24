import { useState, useCallback, useEffect } from 'react';

export const useBalance = () => {
  const [balance, setBalance] = useState(0);
  const [usdValue, setUsdValue] = useState(0);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const fetchBalance = useCallback(async (): Promise<number> => {
    setIsLoading(true);
    setError(null);

    try {
      // Call C++ bridge via window.bitcoinAPI
      if (!window.bitcoinAPI) {
        throw new Error('Bitcoin API not available');
      }

      const response = await window.bitcoinAPI.getBalance({
        address: '1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa' // TODO: Get from wallet
      });

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
      // Try multiple price sources
      console.log('🔍 Fetching BSV price...');

      // Try BSV-specific price sources
      const priceSources = [
        // CoinCap API (has BSV support)
        'https://api.coincap.io/v2/assets/bitcoin-sv',
        // CryptoCompare API (has BSV support)
        'https://min-api.cryptocompare.com/data/price?fsym=BSV&tsyms=USD',
        // CoinGecko as fallback (via CORS proxy)
        'https://api.allorigins.win/raw?url=' + encodeURIComponent('https://api.coingecko.com/api/v3/simple/price?ids=bitcoin-sv&vs_currencies=usd'),
        // Direct CoinGecko (might work in CEF)
        'https://api.coingecko.com/api/v3/simple/price?ids=bitcoin-sv&vs_currencies=usd'
      ];

      for (let i = 0; i < priceSources.length; i++) {
        try {
          console.log(`🔍 Trying price source ${i + 1}: ${priceSources[i]}`);

          const response = await fetch(priceSources[i], {
            method: 'GET',
            headers: {
              'Accept': 'application/json',
              'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'
            },
            mode: 'cors'
          });

          if (!response.ok) {
            console.log(`❌ Source ${i + 1} failed with status: ${response.status}`);
            continue;
          }

          const data = await response.json();
          console.log(`🔍 Source ${i + 1} response:`, data);

          let price = 0;

          // Parse different API response formats
          if (i === 0) {
            // CoinCap API format: { data: { priceUsd: "45.50" } }
            price = parseFloat(data.data?.priceUsd) || 0;
          } else if (i === 1) {
            // CryptoCompare API format: { USD: 45.50 }
            price = parseFloat(data.USD) || 0;
          } else {
            // CoinGecko API format: { "bitcoin-sv": { usd: 45.50 } }
            price = data['bitcoin-sv']?.usd || 0;
          }

          console.log(`🔍 BSV price from source ${i + 1}:`, price);

          if (price > 0) {
            const usdValue = (balance / 100000000) * price; // Convert satoshis to BSV, then to USD
            setUsdValue(usdValue);

            console.log(`💰 BSV Price: $${price}, Balance: ${balance} satoshis, USD Value: $${usdValue.toFixed(2)}`);
            return usdValue;
          }
        } catch (err) {
          console.log(`❌ Source ${i + 1} failed:`, err);
          continue;
        }
      }

      // If all sources fail, return 0 (no USD conversion)
      console.log('⚠️ All price sources failed, no USD conversion available');
      setUsdValue(0);
      return 0;

    } catch (err) {
      console.error('❌ All price fetching failed:', err);
      return 0;
    }
  }, [balance]);

  const refreshBalance = useCallback(async () => {
    await fetchBalance();
    await fetchUsdPrice();
  }, [fetchBalance, fetchUsdPrice]);

  // Auto-refresh balance every 30 seconds
  useEffect(() => {
    const interval = setInterval(() => {
      refreshBalance();
    }, 30000);

    return () => clearInterval(interval);
  }, [refreshBalance]);

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
