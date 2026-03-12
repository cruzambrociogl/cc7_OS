#!/bin/bash
# ============================================================================
# Deploy to BeagleBone Black via Serial/UART (ymodem)
#
# Usage:
#   ./deploy_beagle.sh
#   SERIAL_PORT=/dev/cu.usbserial-XXXX ./deploy_beagle.sh    (macOS)
#   SERIAL_PORT=COM3 ./deploy_beagle.sh                      (Windows)
#
# Flow:
#   1. Builds for hardware
#   2. Waits for you to press RESET on BeagleBone
#   3. Interrupts U-Boot autoboot
#   4. Sends binary via ymodem (with retries)
#   5. Boots the binary
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
LOAD_ADDR="0x80000000"
BINARY="out/beagle/program.bin"

# ---- Check prerequisites ----
# lrzsz-win32 provides sz.exe; Homebrew uses 'lsb'; other systems use 'sb'
if command -v lsb &> /dev/null; then
    SB_CMD="lsb"
elif command -v sb &> /dev/null; then
    SB_CMD="sb"
elif command -v sz &> /dev/null; then
    SB_CMD="sz --ymodem"
else
    echo "ERROR: ymodem send (sb/lsb/sz) not found."
    if $IS_WINDOWS; then
        echo "Install lrzsz-win32: https://github.com/trzsz/lrzsz-win32/releases"
    else
        echo "Install with: brew install lrzsz"
    fi
    exit 1
fi

# ---- Check serial port ----
if $IS_WINDOWS; then
    # On Windows/Git Bash, COMx ports don't show up with -e; use mode command
    if ! /c/WINDOWS/system32/cmd.exe //c "mode $SERIAL_PORT" > /dev/null 2>&1; then
        echo "ERROR: Serial port $SERIAL_PORT not found."
        echo ""
        echo "Available serial ports:"
        /c/WINDOWS/system32/cmd.exe //c "mode" 2>/dev/null | grep "^Status for device COM" || echo "  (none found)"
        echo ""
        echo "Set the correct port COMX"
        # echo "  SERIAL_PORT=COM6 ./deploy_beagle.sh"
        exit 1
    fi
    # Configure baud rate BEFORE opening — Windows COM ports are exclusive
    /c/WINDOWS/system32/cmd.exe //c "mode $SERIAL_PORT baud=$BAUD_RATE parity=n data=8 stop=1" > /dev/null 2>&1
    # Git Bash maps COMx to /dev/ttyS(x-1) for native serial I/O
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

# ---- Step 1: Build for hardware ----
echo "============================================"
echo " Building for BeagleBone Black..."
echo "============================================"
./build_and_run.sh hardware

FILESIZE=$(wc -c < "$BINARY" | tr -d ' ')
echo ""
echo "============================================"
echo " BeagleBone Black Deploy"
echo "============================================"
echo " Binary:  $BINARY ($FILESIZE bytes)"
echo " Serial:  $SERIAL_PORT"
echo " Address: $LOAD_ADDR"
echo "============================================"
echo ""

# ---- Step 2: Open serial and start spamming spaces ----
if $IS_WINDOWS; then
    # Windows: write directly to device (no fd 3 — COM ports don't release properly)
    (while true; do echo -n " " > "$SERIAL_DEV"; sleep 0.1; done) &
    SPAM_PID=$!

    echo ">>> Press RESET on BeagleBone, then press ENTER here <<<"
    read -r

    echo "Interrupting U-Boot autoboot..."
    sleep 2

    kill $SPAM_PID 2>/dev/null || true
    wait $SPAM_PID 2>/dev/null || true
    sleep 0.5

    # ---- Steps 4-7: Python handles loady, ymodem, go, and UART output ----
    echo "Sending $BINARY via ymodem ($FILESIZE bytes)..."
    python "$SCRIPT_DIR/ymodem_send.py" "$SERIAL_PORT" "$BAUD_RATE" "$BINARY" "$LOAD_ADDR"
else
    # macOS: use fd 3 for all serial I/O
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

    echo "Sending loady $LOAD_ADDR..."
    echo "loady $LOAD_ADDR" >&3
    sleep 1

    cat <&3 > /dev/null &
    DRAIN_PID=$!
    sleep 4
    kill $DRAIN_PID 2>/dev/null || true
    wait $DRAIN_PID 2>/dev/null || true

    echo "Sending $BINARY via ymodem ($FILESIZE bytes)..."
    $SB_CMD "$BINARY" <&3 >&3
    sleep 1

    echo "Sending go $LOAD_ADDR..."
    echo "go $LOAD_ADDR" >&3

    echo ""
    echo "============================================"
    echo " Binary deployed and running!"
    echo " Showing UART output (Ctrl+C to disconnect)"
    echo "============================================"
    echo ""

    cat <&3
fi
