#!/usr/bin/env python3
"""Serve only the web assets on loopback and open the flasher. No dependencies."""
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import webbrowser

def main():
    root = Path(__file__).resolve().parents[1] / 'web'
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(SimpleHTTPRequestHandler, directory=str(root)))
    url = f'http://127.0.0.1:{server.server_port}/flasher/'
    print(f'CoralFlower flasher: {url}\nKeep this window open. Ctrl+C stops the server.', flush=True)
    webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()

if __name__ == '__main__':
    main()
