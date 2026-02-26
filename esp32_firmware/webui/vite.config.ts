import path from 'path';
import { defineConfig, loadEnv } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, '.', '');
  return {
    server: {
      port: 3000,
      host: '0.0.0.0',
      // Proxy ESP32 API calls during local development
      proxy: {
        '/api': {
          target: 'http://10.100.9.230',
          changeOrigin: true,
        }
      }
    },
    build: {
      outDir: 'dist',
      // Produce a single-page bundle suitable for SPIFFS/LittleFS flashing
      rollupOptions: {
        output: {
          manualChunks: undefined,
        }
      }
    },
    plugins: [react()],
    resolve: {
      alias: {
        '@': path.resolve(__dirname, '.'),
      }
    }
  };
});
