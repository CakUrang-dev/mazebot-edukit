#!/usr/bin/env python3
"""
MazeBot EduKit - HTTP + WebSocket Server (Python Stdlib Only)
Serves the web dashboard and provides /ws WebSocket endpoint for telemetry.
Compatible with ESP32 AsyncWebServer protocol contract.
"""
import asyncio
import json
import os
import sys
import time
import hashlib
import base64
import secrets
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse
import socket
import threading
import signal

# =============================================================================
# CONFIGURATION
# =============================================================================
HOST = "0.0.0.0"
HTTP_PORT = 8080
WS_PORT = 8081  # WebSocket on separate port (simpler with stdlib)
WEB_ROOT = "/home/cakurang/mazebot-edukit/web"

# =============================================================================
# HTTP SERVER - Serves static files
# =============================================================================
class MazeBotHTTPHandler(BaseHTTPRequestHandler):
    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # Serve index.html for root
        if path == "/" or path == "/index.html":
            self.serve_file("index.html", "text/html")
        elif path == "/health":
            self.send_json({"status": "ok", "server": "MazeBot EduKit HTTP"})
        else:
            # Try to serve as static file
            file_path = path.lstrip("/")
            full_path = os.path.join(WEB_ROOT, file_path)
            if os.path.isfile(full_path):
                ext = os.path.splitext(file_path)[1]
                mime = self.get_mime(ext)
                self.serve_file(file_path, mime)
            else:
                self.send_error(404, "Not Found")

    def serve_file(self, rel_path, mime_type):
        full_path = os.path.join(WEB_ROOT, rel_path)
        try:
            with open(full_path, "rb") as f:
                content = f.read()
            self.send_response(200)
            self.send_header("Content-Type", mime_type)
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Cache-Control", "public, max-age=31536000" if mime_type != "text/html" else "no-cache")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(content)
        except Exception as e:
            self.send_error(500, str(e))

    def send_json(self, data):
        content = json.dumps(data).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(content)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(content)

    def get_mime(self, ext):
        mimes = {
            ".html": "text/html",
            ".css": "text/css",
            ".js": "application/javascript",
            ".json": "application/json",
            ".png": "image/png",
            ".jpg": "image/jpeg",
            ".svg": "image/svg+xml",
            ".ico": "image/x-icon",
            ".gz": "application/gzip",
        }
        return mimes.get(ext, "application/octet-stream")

    def log_message(self, format, *args):
        # Suppress default log noise
        pass


# =============================================================================
# WEBSOCKET SERVER - Stdlib implementation
# =============================================================================
class WebSocketServer:
    def __init__(self, host="0.0.0.0", port=8081):
        self.host = host
        self.port = port
        self.clients = set()
        self.running = False
        self.server_socket = None

    def start(self):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)
        self.running = True
        
        print(f"[WS] WebSocket server listening on ws://{self.host}:{self.port}")
        
        accept_thread = threading.Thread(target=self._accept_loop, daemon=True)
        accept_thread.start()
        
        # Start telemetry broadcast loop
        telemetry_thread = threading.Thread(target=self._telemetry_loop, daemon=True)
        telemetry_thread.start()

    def _accept_loop(self):
        while self.running:
            try:
                client_sock, addr = self.server_socket.accept()
                client_thread = threading.Thread(
                    target=self._handle_client, 
                    args=(client_sock, addr), 
                    daemon=True
                )
                client_thread.start()
            except OSError:
                break

    def _handle_client(self, sock, addr):
        """Handle WebSocket handshake and frames (simplified)"""
        try:
            # Read HTTP upgrade request
            request = b""
            while b"\r\n\r\n" not in request:
                chunk = sock.recv(1024)
                if not chunk:
                    return
                request += chunk

            # Parse WebSocket key
            headers = request.decode("utf-8", errors="ignore")
            ws_key = None
            for line in headers.split("\r\n"):
                if line.lower().startswith("sec-websocket-key:"):
                    ws_key = line.split(":")[1].strip()
                    break

            if not ws_key:
                sock.close()
                return

            # Compute accept key
            GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
            accept_key = base64.b64encode(
                hashlib.sha1((ws_key + GUID).encode()).digest()
            ).decode()

            # Send 101 Switching Protocols
            response = (
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                f"Sec-WebSocket-Accept: {accept_key}\r\n\r\n"
            )
            sock.send(response.encode())

            # Add to clients
            self.clients.add(sock)
            print(f"[WS] Client connected: {addr} (total: {len(self.clients)})")

            # Read frames (we only need to keep connection alive)
            while self.running:
                # Read frame header (2 bytes min)
                header = sock.recv(2)
                if not header or len(header) < 2:
                    break
                
                # Parse frame (simplified - just keep alive)
                fin = header[0] & 0x80
                opcode = header[0] & 0x0F
                masked = header[1] & 0x80
                payload_len = header[1] & 0x7F

                if opcode == 0x8:  # Close frame
                    break

                # Skip payload
                if payload_len == 126:
                    ext_len = sock.recv(2)
                    payload_len = int.from_bytes(ext_len, "big")
                elif payload_len == 127:
                    ext_len = sock.recv(8)
                    payload_len = int.from_bytes(ext_len, "big")

                if masked:
                    mask = sock.recv(4)
                sock.recv(payload_len)  # discard payload

        except Exception as e:
            pass
        finally:
            self.clients.discard(sock)
            try:
                sock.close()
            except:
                pass
            print(f"[WS] Client disconnected: {addr} (total: {len(self.clients)})")

    def _telemetry_loop(self):
        """Broadcast simulated ESP32 telemetry to all connected clients"""
        # Simulated state (matches ESP32 contract)
        state = {
            "status": "IDLE",
            "timer_ms": 0,
            "sensors": {
                "left_cm": 25.0,
                "front_cm": 30.0,
                "right_cm": 5.0,
                "line": 0
            },
            "maze": {
                "grid_count": 0,
                "raw_array": [],
                "optimized_array": []
            }
        }

        # LSRB turn sequences for simulation
        mapping_turns = ['L', 'S', 'L', 'U', 'L', 'R', 'S', 'L', 'R', 'S']
        turn_idx = 0
        last_mission_time = 0
        mission_active = False

        while self.running:
            time.sleep(0.1)  # 10 Hz

            if not self.clients:
                continue

            # Update simulation state
            current_time = int(time.time() * 1000)

            if mission_active:
                state["timer_ms"] += 100

                # Simulate sensor readings
                state["sensors"]["left_cm"] = round(4.8 + (current_time % 3000) / 1000 * 1.5, 1)
                state["sensors"]["front_cm"] = round(18.0 + (current_time % 4000) / 1000 * 12.0, 1)
                state["sensors"]["right_cm"] = round(22.0 + (current_time % 5000) / 1000 * 5.0, 1)

                # Add turns periodically during MAPPING
                if state["status"] == "MAPPING" and current_time - last_mission_time > 2000:
                    last_mission_time = current_time
                    state["maze"]["grid_count"] += 1
                    if turn_idx < len(mapping_turns):
                        state["maze"]["raw_array"].append(mapping_turns[turn_idx])
                        turn_idx += 1
                        state["maze"]["optimized_array"] = self._optimize_lsrb(state["maze"]["raw_array"])

                # Finish line at ~16 seconds
                if state["timer_ms"] >= 16000:
                    state["sensors"]["line"] = 1
                    state["status"] = "STOPPED"
                    mission_active = False

            # Broadcast to all clients
            message = self._create_ws_frame(json.dumps(state).encode())
            dead_clients = set()
            for client in self.clients:
                try:
                    client.send(message)
                except:
                    dead_clients.add(client)
            
            for dc in dead_clients:
                self.clients.discard(dc)

    def _optimize_lsrb(self, path):
        """LSRB Dead-End Elimination Algorithm"""
        if len(path) < 3:
            return path[:]
        optimized = []
        i = 0
        while i < len(path):
            if i <= len(path) - 3 and path[i + 1] == 'U':
                rule = path[i] + 'U' + path[i + 2]
                repl = None
                if rule == 'LUL': repl = 'S'
                elif rule == 'LUS': repl = 'R'
                elif rule == 'RUL': repl = 'U'
                elif rule == 'SUL': repl = 'R'
                elif rule == 'SUS': repl = 'U'
                elif rule == 'LUR': repl = 'U'
                if repl:
                    optimized.append(repl)
                    i += 3
                    continue
            optimized.append(path[i])
            i += 1
        return optimized

    def _create_ws_frame(self, payload):
        """Create WebSocket frame (text, unmasked server->client)"""
        length = len(payload)
        if length < 126:
            header = bytes([0x81, length])
        elif length < 65536:
            header = bytes([0x81, 126]) + length.to_bytes(2, "big")
        else:
            header = bytes([0x81, 127]) + length.to_bytes(8, "big")
        return header + payload

    def stop(self):
        self.running = False
        if self.server_socket:
            self.server_socket.close()
        for client in list(self.clients):
            try:
                client.close()
            except:
                pass


# =============================================================================
# MAIN ENTRY POINT
# =============================================================================
def run_http_server():
    os.chdir(WEB_ROOT)
    server = HTTPServer((HOST, HTTP_PORT), MazeBotHTTPHandler)
    print(f"[HTTP] Serving dashboard at http://{HOST}:{HTTP_PORT}")
    server.serve_forever()

def run_ws_server():
    ws_server = WebSocketServer(HOST, WS_PORT)
    ws_server.start()
    # Keep alive
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        ws_server.stop()

if __name__ == "__main__":
    print("=" * 50)
    print("  MazeBot EduKit - Web Platform Server")
    print("  HTTP Dashboard + WebSocket Telemetry")
    print("=" * 50)

    # Start HTTP server in thread
    http_thread = threading.Thread(target=run_http_server, daemon=True)
    http_thread.start()

    # Run WS server in main thread
    run_ws_server()