#!/usr/bin/env python3

import asyncio
import contextlib
import json
import signal
import sys
import time
from typing import Any, Dict, Optional

import websockets

URI = "ws://localhost/wsprrypi/socket"

RECONNECT_DELAY_S = 5
PING_EVERY_S = 15

shutdown_event = asyncio.Event()


def log(msg: str) -> None:
    print(msg, flush=True)


def parse_json(s: str) -> Optional[Dict[str, Any]]:
    try:
        obj = json.loads(s)
        return obj if isinstance(obj, dict) else None
    except Exception:
        return None


async def handle_message(msg: Dict[str, Any]) -> None:
    if "tx_state" in msg:
        tx_state = msg.get("tx_state")
        state = "transmitting" if tx_state == "transmitting" else "connected"
        log(f"STATE: {state} (tx_state={tx_state})")
        return

    if msg.get("type") == "transmit":
        st = msg.get("state")
        ts = msg.get("timestamp", "")
        log(f"EVENT: transmit {st} @ {ts}")
        return

    if msg.get("type") == "configuration" and msg.get("state") == "reload":
        log("EVENT: configuration reload requested")
        return

    log(f"RX: {msg}")


async def send_command(ws, payload: Any) -> None:
    data = json.dumps({"command": payload})
    await ws.send(data)
    log(f"TX: {data}")


def request_shutdown() -> None:
    # Ensure output starts on a fresh line when Ctrl-C is pressed.
    if not shutdown_event.is_set():
        print("", flush=True)
    shutdown_event.set()


def install_signal_handlers(loop: asyncio.AbstractEventLoop) -> None:
    for sig in (signal.SIGINT, signal.SIGTERM):
        with contextlib.suppress(NotImplementedError):
            loop.add_signal_handler(sig, request_shutdown)


async def receiver_loop(ws) -> None:
    async for raw in ws:
        if shutdown_event.is_set():
            return

        ts = time.strftime("%Y-%m-%d %H:%M:%S")
        msg = parse_json(raw)
        if msg is None:
            log(f"{ts} RX (non-JSON): {raw}")
            continue

        log(f"{ts} RX JSON: {msg}")
        await handle_message(msg)


async def keepalive_loop(ws) -> None:
    while not shutdown_event.is_set():
        t0 = time.perf_counter()
        pong_waiter = await ws.ping()
        await pong_waiter
        rtt_ms = (time.perf_counter() - t0) * 1000.0
        log(f"PING: rtt={rtt_ms:.1f} ms")
        await asyncio.sleep(PING_EVERY_S)


async def input_loop(ws, line_queue: "asyncio.Queue[str]") -> None:
    log("Enter commands. Examples: get_tx_state | reboot | shutdown | exit")

    while not shutdown_event.is_set():
        # Wait for either a line or shutdown.
        line_task = asyncio.create_task(line_queue.get())
        stop_task = asyncio.create_task(shutdown_event.wait())

        done, pending = await asyncio.wait(
            {line_task, stop_task},
            return_when=asyncio.FIRST_COMPLETED,
        )

        for t in pending:
            t.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await t

        if stop_task in done:
            return

        line = line_task.result().strip()
        if not line:
            continue

        if line.lower() in {"exit", "quit"}:
            shutdown_event.set()
            return

        # If user typed a full JSON object, send it raw.
        if line.startswith("{") and line.endswith("}"):
            try:
                json.loads(line)
                await ws.send(line)
                log(f"TX RAW JSON: {line}")
                continue
            except Exception:
                pass

        await send_command(ws, line)


async def run_session() -> None:
    loop = asyncio.get_running_loop()
    line_queue: asyncio.Queue[str] = asyncio.Queue()

    def on_stdin_ready() -> None:
        # Called by the event loop when stdin has data.
        s = sys.stdin.readline()
        if s == "":
            # EOF
            shutdown_event.set()
            return
        line_queue.put_nowait(s)

    # Register stdin reader (Linux/Unix).
    loop.add_reader(sys.stdin, on_stdin_ready)

    try:
        async with websockets.connect(
            URI,
            ping_interval=None,  # manual keepalive
            close_timeout=5,
            max_size=2**20,
        ) as ws:
            log(f"Connected: {URI}")
            await send_command(ws, "get_tx_state")

            tasks = [
                asyncio.create_task(receiver_loop(ws)),
                asyncio.create_task(keepalive_loop(ws)),
                asyncio.create_task(input_loop(ws, line_queue)),
            ]

            # Wait for shutdown
            await shutdown_event.wait()

            # Gracefully close the websocket first
            with contextlib.suppress(Exception):
                await ws.close(code=1000, reason="client shutdown")

            for task in tasks:
                task.cancel()
                with contextlib.suppress(asyncio.CancelledError):
                    await task

    finally:
        # Always remove the stdin reader.
        with contextlib.suppress(Exception):
            loop.remove_reader(sys.stdin)


async def main() -> None:
    loop = asyncio.get_running_loop()
    install_signal_handlers(loop)

    while not shutdown_event.is_set():
        try:
            await run_session()
        except Exception as e:
            if shutdown_event.is_set():
                break
            log(f"Disconnected: {e!r}")
            log(f"Reconnecting in {RECONNECT_DELAY_S}s")
            await asyncio.sleep(RECONNECT_DELAY_S)

    log("Shutting down cleanly.")


if __name__ == "__main__":
    asyncio.run(main())