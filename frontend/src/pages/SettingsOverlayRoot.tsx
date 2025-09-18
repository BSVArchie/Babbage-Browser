import React, { useState, useEffect } from 'react';
import SettingsPanelLayout from '../components/panels/SettingsPanelLayout';

const SettingsOverlayRoot: React.FC = () => {
  const [settingsOpen, setSettingsOpen] = useState(true);

  useEffect(() => {
    console.log("🔧 SettingsOverlayRoot mounted");
    console.log("🔧 cefMessage available:", !!window.cefMessage);
    console.log("🔧 cefMessage.send available:", !!(window.cefMessage && window.cefMessage.send));

    // Test cefMessage immediately
    if (window.cefMessage && window.cefMessage.send) {
      console.log("🔧 Testing cefMessage.send from settings overlay");
      window.cefMessage.send('test_settings_message', []);
    } else {
      console.log("❌ cefMessage not available in settings overlay");
    }

    // Auto-open settings panel when this component mounts
    console.log("🔧 Setting settingsOpen to true");
    setSettingsOpen(true);

    // Set up window trigger for settings panel
    window.triggerPanel = (panelName: string) => {
      console.log("🔧 Settings panel trigger received:", panelName);
      if (panelName === 'settings') {
        setSettingsOpen(true);
      }
    };
  }, []);

  console.log("🔧 SettingsOverlayRoot render - settingsOpen:", settingsOpen);

  return (
    <>
      <SettingsPanelLayout
        open={settingsOpen}
        onClose={() => {
          console.log("🔧 Settings closing");
          setSettingsOpen(false);
          // Use the proper overlay close method
          if (window.bitcoinBrowser && window.bitcoinBrowser.overlay && window.bitcoinBrowser.overlay.close) {
            console.log("🔧 Calling bitcoinBrowser.overlay.close()");
            window.bitcoinBrowser.overlay.close();
          } else {
            console.log("❌ bitcoinBrowser.overlay.close() not available");
            console.log("🔧 bitcoinBrowser:", window.bitcoinBrowser);
            console.log("🔧 bitcoinBrowser.overlay:", window.bitcoinBrowser?.overlay);
          }
        }}
      />
    </>
  );
};

export default SettingsOverlayRoot;
