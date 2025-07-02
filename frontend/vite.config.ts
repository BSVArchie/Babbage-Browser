import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    host: '127.0.0.1',  // 👈 Bind to loopback explicitly
    port: 5137,
    cors: true
  }
})

