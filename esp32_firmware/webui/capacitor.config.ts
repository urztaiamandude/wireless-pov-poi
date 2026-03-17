import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'com.nebulapoi.mobile',
  appName: 'Nebula POV Poi',
  webDir: 'dist',
  server: {
    // ESP32 firmware is served over local HTTP (e.g., http://192.168.4.1).
    cleartext: true,
  },
};

export default config;
