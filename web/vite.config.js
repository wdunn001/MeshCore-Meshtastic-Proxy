import { defineConfig } from 'vite';

export default defineConfig({
  root: '.',
  server: {
    port: 8001,
    open: true,
    host: true,
    strictPort: false,
    hmr: {
      overlay: true
    }
  },
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    emptyOutDir: true,
    rollupOptions: {
      input: './index.html'
    }
  },
  // Ensure WebSerial and other browser APIs work in dev mode
  define: {
    'process.env': {}
  }
});
