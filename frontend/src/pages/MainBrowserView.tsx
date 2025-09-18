import React, { useState } from 'react';
import {
  Box,
  Toolbar,
  IconButton,
  InputBase,
  Paper,
  Typography,
} from '@mui/material';
import DataObjectIcon from '@mui/icons-material/DataObject';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import ArrowForwardIcon from '@mui/icons-material/ArrowForward';
import AccountBalanceWalletIcon from '@mui/icons-material/AccountBalanceWallet';
import MoreVertIcon from '@mui/icons-material/MoreVert';
// Settings panel now rendered in separate overlay process
import { useBitcoinBrowser } from '../hooks/useBitcoinBrowser';


const MainBrowserView: React.FC = () => {
    console.log("🔍 MainBrowserView rendering");
    console.warn("⚠️ Console warn test");
    console.error("❌ Console error test");

    // Test alert
    // alert("MainBrowserView is working!");


    // Settings panel state now managed in separate overlay process
    const [address, setAddress] = useState('https://example.com');

    const { navigate } = useBitcoinBrowser();

    const handleNavigate = () => {
        console.log('🧭 Navigating to:', address);
        navigate(address);
    };

    const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
        if (e.key === 'Enter') {
        handleNavigate();
        }
    };

    return (
        <Box>
            {/* Top Navigation Bar */}
            <Toolbar sx={{ bgcolor: 'grey.100', borderBottom: '1px solid #ccc' }}>
                <IconButton>
                    <ArrowBackIcon />
                </IconButton>
                <IconButton>
                    <ArrowForwardIcon />
                </IconButton>

                {/* Address Bar */}
                <Paper
                    sx={{
                        display: 'flex',
                        alignItems: 'center',
                        flexGrow: 1,
                        mx: 2,
                        height: 40,
                        borderRadius: 20,
                        pl: 2,
                        bgcolor: 'white',
                        boxShadow: 1,
                    }}
                >
                    <InputBase
                        value={address}
                        onChange={(e) => setAddress(e.target.value)}
                        onKeyDown={handleKeyDown}
                        placeholder="Enter address or search"
                        fullWidth
                    />
                </Paper>

                {/* Spacer */}
                <Box flexGrow={1} />

                {/* Wallet Button */}
                <IconButton
                    onClick={() => {
                        console.log("🟢 Wallet button clicked");
                        window.cefMessage?.send('overlay_show_wallet', []);
                        window.bitcoinBrowser.overlay.toggleInput(true);
                        window.bitcoinBrowser.overlayPanel.open('wallet');
                    }}
                    sx={{
                        ml: 1,
                        bgcolor: 'grey.200',
                        borderRadius: '50%',
                        '&:hover': { bgcolor: 'grey.300' }
                    }}
                >
                    <AccountBalanceWalletIcon />
                </IconButton>


                {/* Settings Button */}
                <IconButton
                    onClick={() => {
                        console.log("🔧 Settings button clicked");
                        console.log("🔧 bitcoinBrowser:", window.bitcoinBrowser);
                        console.log("🔧 overlayPanel:", window.bitcoinBrowser?.overlayPanel);
                        console.log("🔧 overlayPanel.toggleInput:", window.bitcoinBrowser?.overlayPanel?.toggleInput);
                        window.cefMessage?.send('overlay_show_settings', []);
                        console.log("🔧 Settings overlay will open in separate process");
                        window.bitcoinBrowser.overlayPanel?.open('settings');
                        window.bitcoinBrowser.overlayPanel?.toggleInput(true);
                    }}
                    sx={{
                        ml: 1,
                        bgcolor: 'grey.200',
                        borderRadius: '50%',
                        '&:hover': { bgcolor: 'grey.300' }
                    }}
                >
                    <MoreVertIcon />
                </IconButton>
            </Toolbar>

            {/* Main Content */}
            <Box p={3}>
            <Typography variant="h5">Welcome to Bitcoin Browser</Typography>
            <Typography variant="body1" mt={2}>
                Address bar, tabs, and stuff will go here.
            </Typography>
            </Box>

            {/* Settings panel is now rendered in separate overlay process */}
        </Box>
    );
};

export default MainBrowserView;
