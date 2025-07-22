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
      overlay: {
        show: () => void;
        hide: () => void;
        toggleInput: (enable: boolean) => void;
      };
      overlayPanel: {
        open: (panelName: string) => void;
      };
    };
    cefMessage?: {
      send: (channel: string, args: any[]) => void;
    };
    triggerPanel?: (panelName: string) => void;
     __overlayReady?: boolean;
  }
}

export {};
