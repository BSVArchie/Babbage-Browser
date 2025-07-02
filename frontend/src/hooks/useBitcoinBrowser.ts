import { useCallback } from 'react';
import type { IdentityResult } from '../types/identity';

export function useBitcoinBrowser() {
  const getIdentity = useCallback(async (): Promise<IdentityResult> => {
    if (!window.bitcoinBrowser?.identity?.get) {
      throw new Error('bitcoinBrowser.identity.get not available');
    }
    const result = await window.bitcoinBrowser.identity.get();
    return result;
  }, []);

  const markBackedUp = useCallback(async (): Promise<string> => {
    if (!window.bitcoinBrowser?.identity?.markBackedUp) {
      throw new Error('bitcoinBrowser.identity.markBackedUp not available');
    }
    const result = await window.bitcoinBrowser.identity.markBackedUp();
    return result;
  }, []);

  const navigate = useCallback((path: string): void => {
    if (!window.bitcoinBrowser?.navigation?.navigate) {
      console.warn('bitcoinBrowser.navigation.navigate not available');
      return;
    }
    try {
      window.bitcoinBrowser.navigation.navigate(path);
    } catch (err) {
      console.error("Navigation error:", err);
    }
  }, []);

  return {
    getIdentity,
    markBackedUp,
    navigate,
  };
}
