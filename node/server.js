const WebSocket = require('ws');

const serverUrl = 'wss://paris4.taming.io/paris9';
const ws = new WebSocket(serverUrl);

ws.on('open', () => {
    console.log(`Connected to ${serverUrl}`);
});

ws.on('message', (data) => {
    console.log(`Received: ${data}`);
});

ws.on('close', () => {
    console.log('Connection closed');
});

ws.on('error', (error) => {
    console.error(`WebSocket error: ${error.message}`);
});
