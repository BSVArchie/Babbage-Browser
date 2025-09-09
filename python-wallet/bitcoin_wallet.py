import json
import os
from pathlib import Path
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from bsv import PrivateKey, PublicKey

class BitcoinWallet:
    def __init__(self, password: str):
        self.password = password
        self.identity_file_path = self._get_identity_file_path()

        # Check if wallet exists, if not create one
        if not self.wallet_exists():
            self._create_new_wallet()
        else:
            self._load_wallet()

    def _get_identity_file_path(self) -> Path:
        import os
        appdata_dir = Path(os.environ.get('APPDATA', ''))
        wallet_dir = appdata_dir / "BabbageBrowser" / "wallet"
        wallet_dir.mkdir(parents=True, exist_ok=True)
        return wallet_dir / "identity.json"

    def wallet_exists(self) -> bool:
        """Check if identity.json exists"""
        return self.identity_file_path.exists()

    def _create_new_wallet(self):
        """Create a new wallet with key pair"""
        print("Creating new Bitcoin wallet...")

        # Generate new key pair
        private_key = PrivateKey()
        public_key = private_key.public_key()  # Call the method to get PublicKey object
        address = private_key.address()  # Address is created from PrivateKey

        # Save identity to file
        self._save_identity_to_file(private_key, public_key, address)

        print(f"✅ Wallet created successfully!")
        print(f"Address: {address}")
        print(f"Identity file: {self.identity_file_path}")

    def _save_identity_to_file(self, private_key: PrivateKey, public_key: PublicKey, address: str):
        """Save wallet identity to encrypted file"""
        # Encrypt private key
        encrypted_private_key = self._encrypt_private_key(private_key.hex())

        identity_data = {
            "publicKey": public_key.hex(),  # Now this should work
            "address": address,
            "privateKey": encrypted_private_key,
            "backedUp": False
        }

        with open(self.identity_file_path, 'w') as f:
            json.dump(identity_data, f, indent=2)

    def _encrypt_private_key(self, private_key_hex: str) -> str:
        """Encrypt private key using PBKDF2 + Fernet"""
        # Derive key from password
        salt = os.urandom(16)
        kdf = PBKDF2HMAC(
            algorithm=hashes.SHA256(),
            length=32,
            salt=salt,
            iterations=100000,
        )
        key = kdf.derive(self.password.encode())

        # Encrypt with Fernet
        f = Fernet(Fernet.generate_key())
        encrypted = f.encrypt(private_key_hex.encode())

        # Return salt + encrypted data as hex
        return (salt + encrypted).hex()

    def _load_wallet(self):
        """Load existing wallet from file"""
        print("Loading existing Bitcoin wallet...")
        # TODO: Implement wallet loading
        pass


