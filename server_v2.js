const express = require('express');
const dgram = require('dgram');
const { exec } = require('child_process');
const fs = require('fs');

const udpServer = dgram.createSocket('udp4');
const app = express();
const port = 3000;
let esp_port;
let esp_addr;

udpServer.on('message', (msg, rinfo) => {
    console.log(`Received UDP message from ${rinfo.address}:${rinfo.port}: ${msg}`);
    esp_port = rinfo.port;
    esp_addr = rinfo.address;

    if (msg.toString() === '1') {
        // Execute the Python script when the message is '1'
        exec('python detect_v3.py', (error, stdout, stderr) => {
            if (error) {
                console.error(`exec error: ${error}`);
                udpServer.send('-1', esp_port, esp_addr); // Send error signal
                return;
            }
            try {
                const result = JSON.parse(stdout);
                let msgToEsp = Buffer.from(`${result.label}`); // Send numeric label directly
                udpServer.send(msgToEsp, esp_port, esp_addr);
                console.log('Sending classification index: ', msgToEsp.toString());
                console.log('Classification result: ', result); // Print the entire result
            } catch (parseError) {
                console.error('Error parsing Python output:', parseError);
                udpServer.send('-1', esp_port, esp_addr); // Send error signal
            }
        });
    } else {
        // Send a negative sign if any other message is received
        udpServer.send('-', esp_port, esp_addr);
    }
});

udpServer.on('error', (err) => {
    console.error(`UDP server error:\n${err.stack}`);
    udpServer.close();
});

udpServer.on('listening', () => {
    const address = udpServer.address();
    console.log(`UDP server listening on ${address.address}:${address.port}`);
});

udpServer.bind(3333); // UDP port to listen on

app.listen(port, () => {
    console.log(`Server running on http://localhost:${port}`);
});
