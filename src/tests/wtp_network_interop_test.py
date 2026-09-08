#!/usr/bin/env python3
"""Run owned clients against the actual Pico server on loopback; bounded restarts."""
import pathlib
import selectors
import subprocess
import sys
import time

build = pathlib.Path(sys.argv[1]).resolve()
server = client = None

def stop(process):
    if not process: return
    process.terminate()
    try: process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()

def start(boot='0', device='a'):
    process = subprocess.Popen([str(build / 'pico_tls_server'), boot, device], stdout=subprocess.PIPE, text=True)
    with selectors.DefaultSelector() as selector:
        selector.register(process.stdout, selectors.EVENT_READ)
        if not selector.select(15) or not process.stdout.readline().startswith('READY'):
            stop(process)
            raise RuntimeError('Pico loopback host server did not start')
    return process

try:
    server = start()
    client = subprocess.Popen([str(build / 'wtp_network_interop_test'), str(build / 'credentials')],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    deadline = time.monotonic() + 180
    with selectors.DefaultSelector() as selector:
        selector.register(client.stdout, selectors.EVENT_READ)
        while client.poll() is None:
            if time.monotonic() > deadline: raise RuntimeError('Interop deadline')
            if not selector.select(1): continue
            line = client.stdout.readline()
            print(line, end='', flush=True)
            if line.startswith('RESTART '):
                stop(server)
                server = start('1', 'b' if 'device' in line else 'a')
                client.stdin.write('READY\n'); client.stdin.flush()
    assert client.wait() == 0, 'Pico interoperability client failed'
finally:
    stop(client)
    stop(server)
