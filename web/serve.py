#!/usr/bin/env python3
"""
TinyKernel Web Server (SisTer Interface)
Lightweight HTTP server with auto-port finding and clean logging.
"""

import http.server
import socketserver
import os
import sys
import webbrowser

PORT = 8080
DIRECTORY = os.path.dirname(os.path.abspath(__file__))

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def end_headers(self):
        self.send_header('Cache-Control', 'no-cache, no-store, must-revalidate')
        self.send_header('Pragma', 'no-cache')
        self.send_header('Expires', '0')
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()

def run(port=PORT, open_browser=False):
    max_attempts = 10
    current_port = port
    
    for _ in range(max_attempts):
        try:
            with socketserver.TCPServer(("", current_port), Handler) as httpd:
                url = f"http://localhost:{current_port}"
                print("=" * 60)
                print("  TinyKernel Web Interface (SisTer Ecosystem)")
                print("  Status: READY ≠ COMPLETE")
                print(f"  URL:    {url}")
                print("=" * 60)
                print("Pressione Ctrl+C para encerrar o servidor.")
                
                if open_browser:
                    webbrowser.open(url)
                
                httpd.serve_forever()
                return
        except OSError:
            current_port += 1

    print(f"Erro: Não foi possível vincular as portas {port}..{current_port-1}", file=sys.stderr)
    sys.exit(1)

if __name__ == "__main__":
    auto_open = "--open" in sys.argv
    p = PORT
    for arg in sys.argv[1:]:
        if arg.isdigit():
            p = int(arg)
    try:
        run(port=p, open_browser=auto_open)
    except KeyboardInterrupt:
        print("\nServidor TinyKernel encerrado.")
