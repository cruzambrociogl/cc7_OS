#!/bin/bash
# ============================================================================
# Deploy Multiprogramming Lab to BeagleBone Black via Serial/UART (ymodem)
#
# Usage:
#   ./deploy_beagle.sh
#   SERIAL_PORT=/dev/cu.usbserial-XXXX ./deploy_beagle.sh    (macOS)
#   SERIAL_PORT=COM3 ./deploy_beagle.sh                      (Windows)
#
# Flow:
#   1. Builds 3 binaries (os.bin, p1.bin, p2.bin)
#   2. Waits for you to press RESET on BeagleBone
#   3. Interrupts U-Boot autoboot
#   4. Sends all 3 binaries via ymodem to their load addresses
#   5. Boots the OS binary
#   6. Stays connected showing UART output
# ============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ---- Detect OS ----
IS_WINDOWS=false
if [[ "$(uname -s)" == MINGW* ]] || [[ "$(uname -s)" == MSYS* ]] || [[ "$(uname -s)" == CYGWIN* ]]; then
    IS_WINDOWS=true
fi

# ---- Configuration ----
if $IS_WINDOWS; then
    SERIAL_PORT="${SERIAL_PORT:-COM6}"
else
    SERIAL_PORT="${SERIAL_PORT:-/dev/cu.usbserial-B001VJVG}"
fi
BAUD_RATE="${BAUD_RATE:-115200}"

# Three binaries with their load addresses
OS_BIN="out/os.bin"
OS_ADDR="0x82000000"
P1_BIN="out/p1.bin"
P1_ADDR="0x82100000"
P2_BIN="out/p2.bin"
P2_ADDR="0x82200000"

# ---- Check prerequisites ----
# On Windows, ymodem_send.py handles all transfers — lrzsz not needed.
# On Linux/macOS, sb/lsb/sz (lrzsz) is required.
if ! $IS_WINDOWS; then
    if command -v lsb &> /dev/null; then
        SB_CMD="lsb"
    elif command -v sb &> /dev/null; then
        SB_CMD="sb"
    elif command -v sz &> /dev/null; then
        SB_CMD="sz --ymodem"
    else
        echo "ERROR: ymodem send (sb/lsb/sz) not found."
        echo "Install with: brew install lrzsz"
        exit 1
    fi
fi

# ---- Check serial port ----
if $IS_WINDOWS; then
    if ! /c/WINDOWS/system32/cmd.exe //c "mode $SERIAL_PORT" > /dev/null 2>&1; then
        echo "ERROR: Serial port $SERIAL_PORT not found."
        echo ""
        echo "Available serial ports:"
        /c/WINDOWS/system32/cmd.exe //c "mode" 2>/dev/null | grep "^Status for device COM" || echo "  (none found)"
        echo ""
        echo "Set the correct port COMX"
        exit 1
    fi
    /c/WINDOWS/system32/cmd.exe //c "mode $SERIAL_PORT baud=$BAUD_RATE parity=n data=8 stop=1" > /dev/null 2>&1
    COM_NUM="${SERIAL_PORT#COM}"
    SERIAL_DEV="/dev/ttyS$((COM_NUM - 1))"
else
    if [ ! -e "$SERIAL_PORT" ]; then
        echo "ERROR: Serial port $SERIAL_PORT not found."
        echo ""
        echo "Available serial ports:"
        ls /dev/cu.usb* 2>/dev/null || echo "  (none found)"
        echo ""
        echo "Set the correct port:"
        echo "  SERIAL_PORT=/dev/cu.YOUR_PORT ./deploy_beagle.sh"
        exit 1
    fi
    SERIAL_DEV="$SERIAL_PORT"
fi

# ---- Step 1: Build all 3 binaries ----
echo "============================================"
echo " Building Multiprogramming Lab..."
echo "============================================"
./build_and_run.sh

OS_SIZE=$(wc -c < "$OS_BIN" | tr -d ' ')
P1_SIZE=$(wc -c < "$P1_BIN" | tr -d ' ')
P2_SIZE=$(wc -c < "$P2_BIN" | tr -d ' ')

echo ""
echo "============================================"
echo " BeagleBone Black Deploy (3 binaries)"
echo "============================================"
echo " OS:  $OS_BIN ($OS_SIZE bytes) → $OS_ADDR"
echo " P1:  $P1_BIN ($P1_SIZE bytes) → $P1_ADDR"
echo " P2:  $P2_BIN ($P2_SIZE bytes) → $P2_ADDR"
echo " Serial: $SERIAL_PORT"
echo "============================================"
echo ""

# ---- Helper: send one binary via ymodem (macOS) ----
send_binary_mac() {
    local binary="$1"
    local addr="$2"
    local name="$3"

    echo "[$name] Sending loady $addr..."
    echo "loady $addr" >&3
    sleep 1

    # Drain serial buffer
    cat <&3 > /dev/null &
    DRAIN_PID=$!
    sleep 4
    kill $DRAIN_PID 2>/dev/null || true
    wait $DRAIN_PID 2>/dev/null || true

    echo "[$name] Sending $binary via ymodem..."
    $SB_CMD "$binary" <&3 >&3
    sleep 1
    echo "[$name] Done."
}

# ---- Step 2: Connect and deploy ----
if $IS_WINDOWS; then
    # On Windows /dev/ttySx is not writable from bash — Python (pyserial) owns
    # the serial port entirely: it opens COM7, spams spaces to interrupt U-Boot,
    # waits for the => prompt, then sends all 3 binaries via ymodem.
    python "$SCRIPT_DIR/ymodem_send.py" "$SERIAL_PORT" "$BAUD_RATE" \
        "$OS_BIN" "$OS_ADDR" "$P1_BIN" "$P1_ADDR" "$P2_BIN" "$P2_ADDR"
else
    exec 3<>"$SERIAL_DEV"
    stty -f "$SERIAL_PORT" "$BAUD_RATE" cs8 -cstopb -parenb raw -echo

    (while true; do echo -n " " >&3; sleep 0.1; done) &
    SPAM_PID=$!

    echo ">>> Press RESET on BeagleBone, then press ENTER here <<<"
    read -r

    echo "Interrupting U-Boot autoboot..."
    sleep 2

    kill $SPAM_PID 2>/dev/null || true
    wait $SPAM_PID 2>/dev/null || true
    sleep 0.5

    # Send all 3 binaries
    send_binary_mac "$OS_BIN" "$OS_ADDR" "OS"
    send_binary_mac "$P1_BIN" "$P1_ADDR" "P1"
    send_binary_mac "$P2_BIN" "$P2_ADDR" "P2"

    # Boot the OS
    echo "Sending go $OS_ADDR..."
    echo "go $OS_ADDR" >&3

    echo ""
    echo "============================================"
    echo " All 3 binaries deployed and running!"
    echo " Showing UART output (Ctrl+C to disconnect)"
    echo "============================================"
    echo ""

    cat <&3
fi
