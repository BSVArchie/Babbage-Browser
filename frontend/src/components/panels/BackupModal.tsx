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
                if (typeof window.bitcoinBrowser.identity?.markBackedUp === 'function') {
                  const result = await window.bitcoinBrowser.identity.markBackedUp();
                  console.log("📝 Backend update result:", result);
                } else {
                  console.warn("❌ markBackedUp is not a function.");
                }
              } catch (err) {
                console.error("💥 Error marking wallet as backed up:", err);
              }

              localStorage.setItem('identity_backed_up', 'true');
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
