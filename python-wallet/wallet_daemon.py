import json
import sys
from bitcoin_wallet import BitcoinWallet

class WalletDaemon:
    def __init__(self):
        self.wallet = None
        self.password = None

    def handle_request(self, request: dict) -> dict:
        """Handle incoming JSON requests from C++ bridge"""
        try:
            action = request.get('action')

            if action == 'init_wallet':
                password = request.get('password', 'default_password')
                self.password = password
                self.wallet = BitcoinWallet(password)
                return {"status": "success", "message": "Wallet initialized"}

            elif action == 'get_identity':
                if not self.wallet:
                    return {"status": "error", "message": "Wallet not initialized"}

                # Get decrypted identity (for testing - in production, this should be more secure)
                identity_data = {
                    "publicKey": self.wallet.identity_file_path.read_text(),
                    "address": "test_address",
                    "backedUp": False
                }
                return {"status": "success", "identity": identity_data}

            elif action == 'mark_backed_up':
                if not self.wallet:
                    return {"status": "error", "message": "Wallet not initialized"}

                # TODO: Implement mark_backed_up
                return {"status": "success", "message": "Wallet marked as backed up"}

            else:
                return {"status": "error", "message": f"Unknown action: {action}"}

        except Exception as e:
            return {"status": "error", "message": str(e)}

def main():
    daemon = WalletDaemon()

    # Read requests from stdin
    for line in sys.stdin:
        try:
            request = json.loads(line.strip())
            response = daemon.handle_request(request)
            print(json.dumps(response))
            sys.stdout.flush()
        except json.JSONDecodeError:
            error_response = {"status": "error", "message": "Invalid JSON"}
            print(json.dumps(error_response))
            sys.stdout.flush()
        except Exception as e:
            error_response = {"status": "error", "message": str(e)}
            print(json.dumps(error_response))
            sys.stdout.flush()

if __name__ == "__main__":
    main()
