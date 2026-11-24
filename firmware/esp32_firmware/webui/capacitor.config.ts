import type { CapacitorConfig } from '@capacitor/cli';

const config: CapacitorConfig = {
  appId: 'com.nebulapoi.mobile',
  appName: 'Nebula POV Poi',
  webDir: 'dist',
  server: {
    // The Android WebView origin is http://localhost, so fetch() to the poi
    // (http://192.168.4.1) is not mixed content. https://localhost would block
    // that cleartext call on current WebViews. CapacitorHttp is left off so
    // FormData uploads and AbortSignal keep using the WebView fetch stack.
    androidScheme: 'http',
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
  },
  android: {
    // Fallback if a WebView still treats the app origin as https.
    allowMixedContent: true,
    backgroundColor: '#020617',
  },
};

export default config;
