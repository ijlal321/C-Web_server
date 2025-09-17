import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  proxy: {
    '/': 'http://localhost:8000',  // just for development. For hot reloading.
  },
  server: {
    port: 4000,
    host: true, // Allow access from other devices on the same network
  }
})
