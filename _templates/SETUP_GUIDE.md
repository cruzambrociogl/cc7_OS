# Dev Environment Setup Guide — Bare-Metal ARM Labs

VS Code build/run/debug environment for bare-metal ARM labs targeting
QEMU (emulation) and BeagleBone Black (hardware). Cross-platform: macOS and Windows.

---

## Overview

Each bare-metal lab gets:

| Feature | How | Where configured |
|---------|-----|-----------------|
| **F5 Debug** (QEMU, breakpoints, step) | Cortex-Debug + GDB + QEMU | `.vscode/launch.json` |
| **F5 Run** (QEMU, no stop) | Same as debug, `stopAtEntry: false` | `.vscode/launch.json` |
| **Beagle: Build & Deploy** | Task running `deploy_beagle.sh` | `.vscode/tasks.json` |
| **QEMU: Build** (Ctrl+Shift+B) | Task running `build_and_run.sh qemu_build_only` | `.vscode/tasks.json` |
| **Stop QEMU** | Task (shared, one for all labs) | `.vscode/tasks.json` |

**Per lab you add: 2 tasks + 2 launch configs.** Use `register_lab.py` to automate this.

---

## Project Structure

```
cc7/                              <-- VS Code workspace root
├── .vscode/
│   ├── tasks.json                <-- All tasks for all labs
│   ├── launch.json               <-- All debug/run configs for all labs
│   └── settings.json             <-- Workspace settings
│
├── 003InterruptHandler_qemu/     <-- Example bare-metal lab
│   └── Code/
│       ├── build_and_run.sh      <-- Build script (QEMU + hardware targets)
│       ├── deploy_beagle.sh      <-- Automated BeagleBone serial deploy
│       ├── ymodem_send.py        <-- Python ymodem handler (Windows deploy)
│       ├── register_lab.py       <-- Adds VS Code configs for this lab
│       ├── linker.ld             <-- BeagleBone linker (loads at 0x80000000)
│       ├── linker_versatile.ld   <-- QEMU VersatilePB linker (loads at 0x10000)
│       ├── bin/                  <-- Build output (program.elf, program.bin)
│       └── OS/
│           ├── root.s            <-- ARM assembly (vector table, startup)
│           └── os.c              <-- C code (main logic, ISRs)
```

---

## Prerequisites

### Both Platforms

| Tool | Purpose |
|------|---------|
| `arm-none-eabi-gcc` / `arm-none-eabi-gdb` | Cross-compiler and debugger |
| `qemu-system-arm` | ARM emulator |
| Cortex-Debug extension (`marus25.cortex-debug`) | GDB + QEMU integration in VS Code |
| C/C++ extension (`ms-vscode.cpptools`) | IntelliSense, syntax highlighting |

### macOS

```bash
brew install arm-none-eabi-gcc qemu lrzsz
```

| Tool | Path |
|------|------|
| `arm-none-eabi-gdb` | `/opt/homebrew/bin/arm-none-eabi-gdb` |
| `qemu-system-arm` | `/opt/homebrew/bin/qemu-system-arm` |
| `lsb` (ymodem send) | `/opt/homebrew/bin/lsb` (from lrzsz) |

### Windows

**Install:**

| Tool | How to Install |
|------|----------------|
| Git for Windows | [git-scm.com](https://git-scm.com) — includes Git Bash (required shell) |
| ARM GNU Toolchain | Download `mingw-w64-i686` `.msi` from [ARM Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) |
| QEMU | [qemu.org/download](https://www.qemu.org/download/#windows) |
| Make | `winget install GnuWin32.Make` |
| Python 3 | [python.org](https://www.python.org/downloads/) — check "Add to PATH" |
| lrzsz | [lrzsz-win32 v0.12.21rc](https://github.com/trzsz/lrzsz-win32/releases) — extract to `C:\lrzsz\` |

**Add to System PATH:**
```
C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin
C:\Program Files\qemu
C:\Program Files (x86)\GnuWin32\bin
C:\lrzsz\lrzsz_0.12.21rc_windows_x86_64
```

**Python packages (for BeagleBone deploy):**
```bash
pip install pyserial ymodem
```

**Verify (Git Bash):**
```bash
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
qemu-system-arm --version
make --version
python --version
```

Restart VS Code after updating PATH.

---

## VS Code Configuration

### launch.json

Uses command names so it works on both platforms (tools must be in PATH):

```jsonc
"gdbPath": "arm-none-eabi-gdb",
"qemuPath": "qemu-system-arm",
```

> On macOS, full Homebrew paths (`/opt/homebrew/bin/...`) also work.

### tasks.json

Stop QEMU is cross-platform (auto-detects at runtime):

```jsonc
{
    "label": "Stop QEMU",
    "type": "shell",
    "command": "if command -v taskkill &>/dev/null; then taskkill /F /IM qemu-system-arm.exe; else killall qemu-system-arm; fi && echo 'QEMU stopped' || echo 'QEMU not running'",
    "problemMatcher": []
}
```

`register_lab.py` generates this automatically.

---

## File Details

### build_and_run.sh (cross-platform, reusable per lab)

```bash
./build_and_run.sh              # Builds for BeagleBone (default)
./build_and_run.sh hardware     # Same as above
./build_and_run.sh qemu         # Builds for QEMU + runs it
./build_and_run.sh qemu_build_only  # Builds for QEMU only (used by F5 debug)
```

- QEMU target: `-mcpu=arm926ej-s`, `-DQEMU`, `linker_versatile.ld`
- Hardware target: `-mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard`, `linker.ld`
- Source code uses `#ifdef QEMU` for platform-specific behavior
- Outputs: `bin/program.elf` (QEMU/GDB) and `bin/program.bin` (BeagleBone)
- No OS-specific code — works in Git Bash and macOS bash

### deploy_beagle.sh (cross-platform, reusable per lab)

Auto-detects OS (MINGW/MSYS/CYGWIN vs macOS) and adapts:

| Step | macOS | Windows |
|------|-------|---------|
| Serial port default | `/dev/cu.usbserial-...` | `COM6` |
| Port config | `stty -f` | `mode` via `cmd.exe` |
| Serial I/O | `exec 3<>` + fd redirection | Direct writes to `/dev/ttyS` |
| Ymodem transfer | `lsb` (lrzsz via Homebrew) | `ymodem_send.py` (pyserial) |
| UART output | `cat <&3` | Python serial read loop |

**Why Python for Windows ymodem?** The lrzsz-win32 `sz.exe` uses a Cygwin runtime
incompatible with Git Bash's MSYS2 environment. Python's `pyserial` handles Windows
COM ports natively.

**Setting the serial port:**
- macOS: `SERIAL_PORT=/dev/cu.YOUR_PORT ./deploy_beagle.sh`
- Windows: `SERIAL_PORT=COM4 ./deploy_beagle.sh`
- Or edit the default in the script

### ymodem_send.py (Windows only, reusable)

Called by `deploy_beagle.sh` on Windows. Handles loady command, ymodem transfer,
go command, and UART output display via pyserial.

### register_lab.py (cross-platform, reusable)

Automates VS Code configuration for new labs:
```bash
cd LabXXX/Code/
python register_lab.py
```

- Reads tasks.json/launch.json (handles JSONC comments)
- Adds 2 tasks + 2 launch configs based on lab folder name
- Generates cross-platform Stop QEMU task
- Safe to re-run — skips existing entries

### Linker Scripts

**linker.ld** (BeagleBone): RAM at `0x80000000`, 256KB, sections: .text, .data, .bss, stack at end

**linker_versatile.ld** (QEMU VersatilePB): Code at `0x10000`, 16KB stack

---

## Adding a New Lab

1. Copy into the new lab's `Code/` directory:
   - `build_and_run.sh` (adapt source file paths if structure differs)
   - `deploy_beagle.sh` (reusable as-is)
   - `ymodem_send.py` (reusable as-is)
   - `register_lab.py` (reusable as-is)
   - `linker.ld`
   - `linker_versatile.ld`

2. Register with VS Code:
   ```bash
   cd NewLab/Code/
   python register_lab.py
   ```

3. Reload VS Code window to pick up changes.

---

## Bugs Fixed During Setup

| Bug | Cause | Fix |
|-----|-------|-----|
| Cortex-Debug wrong QEMU machine | `machine`/`cpu` ignored inside `qemuArgs` | Use separate `"cpu"` and `"machine"` fields |
| `sb` command not found (macOS) | Homebrew names it `lsb` | Detect both `lsb` and `sb` in script |
| Ymodem `No ACK on EOT` | `loadb` = kermit, not ymodem | Changed to `loady` (ymodem protocol) |
| Linker warning `cannot find _start` | Label missing in root.s | Added `_start:` before `vector_table:` |
| Deploy fails if user waits after RESET | Spaces sent too late for U-Boot countdown | Spam spaces in background before RESET prompt |
| `set -e` kills script on drain | `wait` returns killed process exit code | Added `|| true` to kill/wait commands |
| Serial buffer contaminates ymodem (macOS) | U-Boot banner in read buffer | Background `cat` drain for 4s before `lsb` |
| lrzsz `sz.exe` crashes (Windows) | Cygwin runtime conflicts with MSYS2 | Use Python `ymodem_send.py` instead |

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| `command not found` after install | Restart VS Code to pick up PATH changes |
| Serial port not found | macOS: `ls /dev/cu.usb*` / Windows: check Device Manager > Ports (COM & LPT) |
| Close other serial tools | CoolTerm, PuTTY, etc. lock the port exclusively |
| Ymodem "No ACK" | U-Boot didn't receive `loady` — reset the board and try again |
| QEMU won't stop | Run Stop QEMU task, or manually: macOS `killall qemu-system-arm` / Windows `taskkill /F /IM qemu-system-arm.exe` |
| Linker warning "LOAD segment with RWX" | Cosmetic warning, harmless — can be ignored |

---

## Limitations

- **No step-through debugging on BeagleBone** — requires JTAG hardware (~$20-60). UART only carries text.
- **BeagleBone debugging = printf via UART** — use `deploy_beagle.sh` to test on real hardware.
- **QEMU = full debugging** — breakpoints, step, registers, watch variables via F5 Debug.
- **deploy_beagle.sh timing** — ymodem transfer can occasionally fail (macOS). Re-run if it does.
