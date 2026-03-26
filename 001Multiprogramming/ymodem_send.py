#!/usr/bin/env python3
"""Interrupt U-Boot autoboot, send 3 binaries via ymodem, then go.
Used by deploy_beagle.sh on Windows (pyserial owns the COM port entirely).

Usage:
    python ymodem_send.py <PORT> <BAUD> <file1> <addr1> [<file2> <addr2> ...]
"""
import sys
import time
import threading
import serial
from ymodem.Socket import ModemSocket


def interrupt_autoboot(ser, stop_event):
    """Spam spaces on the serial port until stop_event is set."""
    while not stop_event.is_set():
        ser.write(b" ")
        time.sleep(0.05)


def wait_for_prompt(ser, prompt=b"=>", timeout=10.0):
    """Read serial output until U-Boot prompt appears or timeout. Returns True on success."""
    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        chunk = ser.read(ser.in_waiting or 1)
        if chunk:
            buf += chunk
            sys.stdout.buffer.write(chunk)
            sys.stdout.flush()
            if prompt in buf:
                return True
    return False


def send_binary(ser, filepath, load_addr):
    """Issue loady, wait for U-Boot to enter ymodem receive mode, transfer file."""
    print(f"\n[ymodem] loady {load_addr}  <-  {filepath}")

    ser.write(f"loady {load_addr}\r\n".encode())

    # U-Boot prints "## Ready for binary (ymodem) download..."
    # and then starts emitting 'C' to kick off the ymodem handshake.
    # Wait long enough for it to appear — do NOT drain the buffer.
    time.sleep(2.0)

    cli = ModemSocket(
        lambda size, timeout=None: ser.read(size),
        lambda data, timeout=None: ser.write(data)
    )
    cli.send([filepath])
    time.sleep(0.5)

    print(f"[ymodem] {filepath} transferred")


def main():
    if len(sys.argv) < 5 or (len(sys.argv) - 3) % 2 != 0:
        print("Usage: ymodem_send.py <PORT> <BAUD> <file1> <addr1> [<file2> <addr2> ...]")
        sys.exit(1)

    port    = sys.argv[1]
    baud    = int(sys.argv[2])
    pairs   = list(zip(sys.argv[3::2], sys.argv[4::2]))  # [(file, addr), ...]
    go_addr = pairs[0][1]                                 # boot from first binary (OS)

    ser = serial.Serial(port, baud, timeout=1)
    print(f"[deploy] Opened {port} @ {baud} baud")

    # --- Step 1: interrupt U-Boot autoboot ---
    # Start spamming spaces in a background thread immediately.
    # The user presses RESET on BeagleBone while the spam is running,
    # so the first keypress lands during the autoboot countdown.
    stop_spam = threading.Event()
    spam_thread = threading.Thread(target=interrupt_autoboot, args=(ser, stop_spam), daemon=True)
    spam_thread.start()

    input("\n>>> Press RESET on BeagleBone, then press ENTER here <<<\n")

    print("[deploy] Waiting for U-Boot prompt (=>)...")
    got_prompt = wait_for_prompt(ser, prompt=b"=>", timeout=10.0)
    stop_spam.set()

    if not got_prompt:
        print("[deploy] WARNING: U-Boot prompt not seen — autoboot may not have been interrupted.")
        print("         Continuing anyway; retry if transfer fails.")
    else:
        print("[deploy] U-Boot prompt detected.")

    # Flush any leftover chars
    ser.reset_input_buffer()

    # --- Step 2: send each binary via ymodem ---
    for filepath, load_addr in pairs:
        send_binary(ser, filepath, load_addr)

    # --- Step 3: boot the OS ---
    print(f"\n[deploy] go {go_addr}")
    ser.write(f"go {go_addr}\r\n".encode())

    print()
    print("============================================")
    print(" All binaries deployed — BeagleBone running")
    print(" Ctrl+C to disconnect")
    print("============================================")
    print()

    # --- Step 4: stream UART output ---
    ser.timeout = 0.05
    try:
        while True:
            data = ser.read(ser.in_waiting or 1)
            if data:
                sys.stdout.buffer.write(data)
                sys.stdout.flush()
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()


if __name__ == "__main__":
    main()
