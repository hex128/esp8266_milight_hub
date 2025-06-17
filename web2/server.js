const express = require('express');
const { createProxyMiddleware } = require('http-proxy-middleware');
const path = require('path');
const fs = require('fs');

const app = express();
const wsApp = express();
const PORT = 8080; // You can change this to any port you prefer
const WS_PORT = 8000; // You can change this to any port you prefer
const PROXY_URL = process.env.PROXY_URL || 'http://milight-hub.local';
const WS_PROXY_URL = process.env.WS_PROXY_URL || `http://milight-hub.local:8000`;

// HTTP proxy middleware
const httpProxyMiddleware = createProxyMiddleware({
  target: PROXY_URL,
  changeOrigin: true,
});

// WebSocket proxy middleware
const wsProxyMiddleware = createProxyMiddleware({
  target: WS_PROXY_URL,
  changeOrigin: true,
  ws: true, // Enable WebSocket proxying
});

app.use((req, res, next) => {
  if (req.path === '/' || fs.existsSync(path.join(__dirname, 'dist', 'build', req.path))) {
    console.log(`Serving ${req.path} from build folder`);
    return express.static(path.join(__dirname, 'dist', 'build'))(req, res, next);
  } else if (fs.existsSync(path.join(__dirname, 'dist', 'compiled', req.path))) {
    console.log(`Serving ${req.path} from compiled folder`);
    return express.static(path.join(__dirname, 'dist', 'compiled'))(req, res, next);
  } else {
    console.log(`Serving ${req.path} from proxy target`);
    return httpProxyMiddleware(req, res, next);
  }
});

wsApp.use(wsProxyMiddleware);

app.listen(PORT, () => {
  console.log(`Server is running on http://localhost:${PORT}`);
  console.log(`HTTP requests proxied to: ${PROXY_URL}`);
  console.log(`WebSocket connections proxied to: ${WS_PROXY_URL}`);
});

wsApp.listen(WS_PORT, () => {
  console.log(`WebSocket server is running on ws://localhost:${WS_PORT}`);
  console.log(`WebSocket requests proxied to: ${WS_PROXY_URL}`);
});
