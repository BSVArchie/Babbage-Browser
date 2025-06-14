// frontend/src/types.d.ts

export {};

declare global {
  interface Window {
    identity?: {
      get?: () => Promise<IdentityResult>;
      markBackedUp: () => Promise<void>;
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
