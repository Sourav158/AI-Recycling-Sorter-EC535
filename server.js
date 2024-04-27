const express = require('express');
const { exec } = require('child_process');
const path = require('path');

const app = express();
const port = 3000;

app.use(express.static('public'));  

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.get('/classify', (req, res) => {
  exec('python detect_v3.py', (error, stdout, stderr) => {
    if (error) {
      console.error(`exec error: ${error}`);
      return res.send({ error: `exec error: ${error.message}` });
    }
    res.send(stdout);
  });
});

app.listen(port, () => {
  console.log(`Server running on http://localhost:${port}`);
});
