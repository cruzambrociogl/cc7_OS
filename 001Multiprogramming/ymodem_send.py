#!/usr/bin/env python3
"""Send loady + ymodem + go over serial, then show UART output.
Used by deploy_beagle.sh on Windows."""
import sys
import time
import serial
from ymodem.Socket import ModemSocket

def deploy(port, baud, filepath, load_addr):
    ser = serial.Serial(port, baud, timeout=2)

    # Send loady command
    ser.write(f"loady {load_addr}\r\n".encode())
    time.sleep(1)

    # Drain any buffered U-Boot output
    ser.read(ser.in_waiting or 1024)
    time.sleep(0.5)

    # Send file via ymodem
    cli = ModemSocket(lambda size, timeout=None: ser.read(size),
                      lambda data, timeout=None: ser.write(data))
    cli.send([filepath])
    time.sleep(1)

    # Boot the binary
    print(f"Sending go {load_addr}...")
    ser.write(f"go {load_addr}\r\n".encode())

    print()
    print("============================================")
    print(" Binary deployed and running!")
    print(" Showing UART output (Ctrl+C to disconnect)")
    print("============================================")
    print()

    # Show UART output until Ctrl+C
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
    deploy(sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4])
