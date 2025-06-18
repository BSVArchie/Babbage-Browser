// frontend/src/types.d.ts

export {};

export {};

declare global {
  interface Window {
    bitcoinBrowser: {
      identity: {
        get: () => Promise<any>;
        markBackedUp: () => Promise<any>;
        // add other functions as you expose them
      };
    };
  }
}


type IdentityData = {
  publicKey: string;
  privateKey: string;
  address: string;
  backedUp: boolean;
};

type BackupCheck = {
  backedUp: true;
};

export type IdentityResult = IdentityData | BackupCheck;
