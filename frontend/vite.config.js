import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      "/download_chunk": {
        target: "http://localhost:8080", // for dev only, for hot reloading.
        changeOrigin: true
      }
    },
    port: 4000,
    host: true, // Allow access from other devices on the same network
  }
})
