import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'com.nebulapoi.mobile',
  appName: 'Nebula POV Poi',
  webDir: 'dist',
  server: {
    // ESP32 firmware is served over local HTTP (e.g., http://192.168.4.1).
    cleartext: true,
  },
  plugins: {
    Camera: {
      // Save photos to the app's internal storage (no gallery pollution)
      saveToGallery: false,
    },
    StatusBar: {
      // Match the dark slate-900 theme of the web UI
      backgroundColor: '#0f172a',
      style: 'DARK',
    },
    SplashScreen: {
      launchAutoHide: true,
      launchShowDuration: 1500,
      backgroundColor: '#020617', // slate-950
      showSpinner: false,
    },
  },
  android: {
    // Allow mixed content so the WebView can reach http://192.168.4.1
    allowMixedContent: true,
    backgroundColor: '#020617',
  },
};

export default config;
