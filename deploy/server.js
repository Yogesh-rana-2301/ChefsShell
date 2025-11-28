const express = require("express");
const http = require("http");
const WebSocket = require("ws");
const pty = require("node-pty");
const path = require("path");
const fs = require("fs");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server });

const PORT = process.env.PORT || 3000;

// Serve static files (HTML, JS, CSS)
app.use(express.static(path.join(__dirname, "public")));

// Path to the compiled shell binary
const SHELL_BINARY = path.join(__dirname, "..", "chefs_shell");

// Check if shell binary exists
if (!fs.existsSync(SHELL_BINARY)) {
  console.error(` Shell binary not found at: ${SHELL_BINARY}`);
  console.error(
    "Please compile the shell first by running: npm run build-shell"
  );
  process.exit(1);
}

console.log(` Shell binary found at: ${SHELL_BINARY}`);

// Handle WebSocket connections
wss.on("connection", (ws) => {
  console.log("New client connected");

  // Spawn the C shell in a PTY
  const shell = pty.spawn(SHELL_BINARY, [], {
    name: "xterm-256color",
    cols: 80,
    rows: 30,
    cwd: process.env.HOME || "/tmp",
    env: {
      ...process.env,
      TERM: "xterm-256color",
      COLORTERM: "truecolor",
    },
  });

  console.log(` Spawned shell process (PID: ${shell.pid})`);

  // Forward shell output to WebSocket client
  shell.on("data", (data) => {
    try {
      ws.send(data);
    } catch (err) {
      console.error("Error sending data to client:", err.message);
    }
  });

  // Handle shell exit
  shell.on("exit", (code, signal) => {
    console.log(`Shell process exited with code ${code}, signal ${signal}`);
    ws.close();
  });

  // Receive input from WebSocket client and send to shell
  ws.on("message", (message) => {
    try {
      shell.write(message.toString());
    } catch (err) {
      console.error("Error writing to shell:", err.message);
    }
  });

  // Handle terminal resize
  ws.on("message", (message) => {
    try {
      const data = JSON.parse(message);
      if (data.type === "resize") {
        shell.resize(data.cols, data.rows);
      }
    } catch (err) {
      // Not JSON or not a resize message, ignore
    }
  });

  // Clean up on WebSocket close
  ws.on("close", () => {
    console.log(" Client disconnected");
    if (shell && !shell.killed) {
      shell.kill();
    }
  });

  // Handle errors
  ws.on("error", (err) => {
    console.error("WebSocket error:", err.message);
  });
});

server.listen(PORT, () => {
  console.log(`
╔════════════════════════════════════════╗
║     🍳 ChefsShell Web Server 🍳       ║
╚════════════════════════════════════════╝

  Server running at: http://localhost:${PORT}
  Shell binary: ${SHELL_BINARY}
  
  Open your browser and navigate to the URL above!
  `);
});
