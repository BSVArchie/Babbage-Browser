import type { IdentityResult } from './identity';
import type { AddressData } from './address';
import type { TransactionResponse, BroadcastResponse } from './transaction';

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
    bitcoinAPI?: {
      createTransaction: (data: any) => Promise<TransactionResponse>;
      signTransaction: (data: any) => Promise<TransactionResponse>;
      broadcastTransaction: (data: any) => Promise<BroadcastResponse>;
      getBalance: (data: any) => Promise<{ balance: number }>;
      getTransactionHistory: () => Promise<any[]>;
    };
    onCreateTransactionResponse?: (data: TransactionResponse) => void;
    onCreateTransactionError?: (error: string) => void;
    onSignTransactionResponse?: (data: TransactionResponse) => void;
    onSignTransactionError?: (error: string) => void;
    onBroadcastTransactionResponse?: (data: BroadcastResponse) => void;
    onBroadcastTransactionError?: (error: string) => void;
    onGetBalanceResponse?: (data: { balance: number }) => void;
    onGetBalanceError?: (error: string) => void;
    onGetTransactionHistoryResponse?: (data: any[]) => void;
    onGetTransactionHistoryError?: (error: string) => void;
     __overlayReady?: boolean;
  }
}

export {};
