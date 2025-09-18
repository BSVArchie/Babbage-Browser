import type { IdentityResult } from './identity';
import type { AddressData } from './address';

declare global {
  interface Window {
    bitcoinBrowser: {
      identity: {
        get: () => Promise<IdentityResult>;
        markBackedUp: () => Promise<string>;
      };
      address: {
        generate: () => Promise<AddressData>;
      };
      navigation: {
        navigate: (path: string) => void;
      };
      overlay: {
        show: () => void;
        hide: () => void;
        toggleInput: (enable: boolean) => void;
        close: () => void;
      };
      overlayPanel: {
        open: (panelName: string) => void;
        toggleInput: (enable: boolean) => void;
      };
    };
    cefMessage?: {
      send: (channel: string, args: any[]) => void;
    };
    triggerPanel?: (panelName: string) => void;
    onAddressGenerated?: (data: AddressData) => void;
    onAddressError?: (error: string) => void;
     __overlayReady?: boolean;
  }
}

export {};
