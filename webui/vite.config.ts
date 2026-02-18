import path from 'path';
import { fileURLToPath } from 'url';
import { defineConfig, loadEnv } from 'vite';
import react from '@vitejs/plugin-react';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

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
      'process.env.API_KEY': JSON.stringify(env.GEMINI_API_KEY),
      'process.env.GEMINI_API_KEY': JSON.stringify(env.GEMINI_API_KEY)
    },
    resolve: {
      alias: {
        '@': path.resolve(__dirname, '.'),
      }
    }
  };
});
