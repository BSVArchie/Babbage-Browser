import React, { useState } from 'react';
import {
  Modal,
  Box,
  Typography,
  Button,
  IconButton,
  Collapse,
  TextField,
  Checkbox,
  FormControlLabel
} from '@mui/material';
import ContentCopyIcon from '@mui/icons-material/ContentCopy';
import WarningAmberIcon from '@mui/icons-material/WarningAmber';

const style = {
  position: 'absolute' as const,
  top: '50%',
  left: '50%',
  transform: 'translate(-50%, -50%)',
  width: 500,
  bgcolor: 'background.paper',
  borderRadius: 2,
  boxShadow: 24,
  p: 4,
};

type Props = {
  open: boolean;
  onClose: () => void;
  identity: {
    address: string;
    publicKey: string;
    privateKey: string;
  };
};

const BackupModal: React.FC<Props> = ({ open, onClose, identity }) => {
  const [showPrivate, setShowPrivate] = useState(false);
  const [copied, setCopied] = useState(false);
  const [confirmedBackup, setConfirmedBackup] = useState(false);

  console.log("💾 BackupModal render - open:", open, "identity:", identity);

  const handleCopy = (text: string) => {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 2000);
  };

  return (
    <Modal
    open={open}
    onClose={(event, reason) => {
        if (reason === 'backdropClick' || reason === 'escapeKeyDown') {
          // Do nothing, prevent closing
          return;
        }
        onClose();
      }}
      disableEscapeKeyDown
    >
      <Box sx={style}>
        <Typography variant="h6" gutterBottom>
          <WarningAmberIcon sx={{ verticalAlign: 'middle', mr: 1, color: 'orange' }} />
          Backup Your Wallet
        </Typography>
        <Typography variant="body2" color="text.secondary" mb={2}>
          This wallet is stored on your computer. If you lose this device or clear its data, your funds and identity may be lost forever.
        </Typography>

        <TextField
          fullWidth
          label="Address"
          value={identity.address}
          InputProps={{
            endAdornment: (
              <IconButton onClick={() => handleCopy(identity.address)} size="small">
                <ContentCopyIcon fontSize="small" />
              </IconButton>
            ),
            readOnly: true,
          }}
          margin="normal"
        />

        <TextField
          fullWidth
          label="Public Key"
          value={identity.publicKey}
          InputProps={{
            endAdornment: (
              <IconButton onClick={() => handleCopy(identity.publicKey)} size="small">
                <ContentCopyIcon fontSize="small" />
              </IconButton>
            ),
            readOnly: true,
          }}
          margin="normal"
        />

        <Collapse in={showPrivate}>
          <TextField
            fullWidth
            label="Private Key"
            value={identity.privateKey}
            InputProps={{
              endAdornment: (
                <IconButton onClick={() => handleCopy(identity.privateKey)} size="small">
                  <ContentCopyIcon fontSize="small" />
                </IconButton>
              ),
              readOnly: true,
            }}
            margin="normal"
          />
        </Collapse>

        <FormControlLabel
          control={
            <Checkbox
              checked={confirmedBackup}
              onChange={(e) => setConfirmedBackup(e.target.checked)}
              name="confirmBackup"
              color="primary"
            />
          }
          label="I have securely backed up my wallet information."
          sx={{ mt: 2 }}
        />

        <Box mt={2} display="flex" justifyContent="space-between">
          <Button onClick={() => setShowPrivate((prev) => !prev)} color="warning">
            {showPrivate ? 'Hide Private Key' : 'Show Private Key'}
          </Button>
          <Button
            onClick={async () => {
              try {
                console.log("📝 Marking identity as backed up...");

                // Set up response listener
                const handleResponse = (event: any) => {
                  if (event.detail.message === 'mark_identity_backed_up_response') {
                    try {
                      const response = JSON.parse(event.detail.args[0]);
                      console.log("📝 Mark backed up response:", response);

                      if (response.success) {
                        console.log("✅ Identity successfully marked as backed up");
                      } else {
                        console.error("❌ Failed to mark identity as backed up:", response.error);
                      }
                    } catch (error) {
                      console.error("💥 Error parsing mark backed up response:", error);
                    }

                    // Remove listener
                    window.removeEventListener('cefMessageResponse', handleResponse);
                  }
                };

                window.addEventListener('cefMessageResponse', handleResponse);

                // Send mark backed up request
                if (window.cefMessage?.send) {
                  window.cefMessage.send('mark_identity_backed_up', []);
                } else {
                  console.error("❌ cefMessage not available");
                }

                // Cleanup listener after timeout
                setTimeout(() => {
                  window.removeEventListener('cefMessageResponse', handleResponse);
                }, 5000);

              } catch (err) {
                console.error("💥 Error marking wallet as backed up:", err);
              }

              // Close modal regardless of success/failure
              onClose();
            }}
            variant="contained"
            color="primary"
            disabled={!confirmedBackup}
          >
            Done
          </Button>
        </Box>

        {copied && (
          <Typography color="success.main" mt={1} fontSize={14}>
            Copied to clipboard!
          </Typography>
        )}
      </Box>
    </Modal>
  );
};

export default BackupModal;
