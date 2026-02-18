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
          target: 'http://192.168.4.1',
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
    define: {
      // Do not inline the real GEMINI_API_KEY into the client bundle to avoid exposing secrets.
      // Provide empty string placeholders so client code can safely reference these values.
      // For AI features to work, implement a backend proxy that keeps the API key server-side.
      'process.env.API_KEY': JSON.stringify(''),
      'process.env.GEMINI_API_KEY': JSON.stringify('')
    },
    resolve: {
      alias: {
        '@': path.resolve(__dirname, '.'),
      }
    }
  };
});
