import type { IdentityResult } from './identity';

declare global {
  interface Window {
    bitcoinBrowser: {
      identity: {
        get: () => Promise<IdentityResult>;
        markBackedUp: () => Promise<string>;
      };
      navigation: {
        navigate: (path: string) => void;
      };
    };
    cefMessage?: {
      send: (channel: string, args: any[]) => void;
    };
  }
}

export {};
