import React, { useState } from 'react';
import {
  Box,
  Toolbar,
  IconButton,
  InputBase,
  Paper,
  Typography
} from '@mui/material';
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import ArrowForwardIcon from '@mui/icons-material/ArrowForward';
import AccountBalanceWalletIcon from '@mui/icons-material/AccountBalanceWallet';
import MoreVertIcon from '@mui/icons-material/MoreVert';

import WalletPanelLayout from '../components/panels/WalletPanelLayout';
import SettingsPanelLayout from '../components/panels/SettingsPanelLayout';


const MainBrowserView: React.FC = () => {

    const [walletPanelOpen, setWalletPanelOpen] = useState(false);
    const [settingsPanelOpen, setSettingsPanelOpen] = useState(false);
    const [address, setAddress] = useState('https://example.com');

    const handleNavigate = () => {
        console.log('Navigate to:', address);
        // This is where we'll call backend.navigate(address) later.
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
                    component="form"
                    onSubmit={(e) => { e.preventDefault(); handleNavigate(); }}
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
                    onClick={() => setWalletPanelOpen(true)}
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
                    onClick={() => setSettingsPanelOpen(true)}
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
                Address bar, tabs, and dApps will go here.
            </Typography>
            </Box>

            {/* Wallet Panel Drawer */}
            <WalletPanelLayout open={walletPanelOpen} onClose={() => setWalletPanelOpen(false)} />

            {/* Settings Panel Drawer */}
            <SettingsPanelLayout open={settingsPanelOpen} onClose={() => setSettingsPanelOpen(false)} />
        </Box>
    );
};

export default MainBrowserView;
