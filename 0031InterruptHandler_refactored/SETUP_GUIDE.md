# Lab Folder Structure — HAL/BSP Architecture

Bare-metal ARM lab structure that separates platform-specific code from
application logic. Targets QEMU (VersatilePB) and BeagleBone Black (AM335x)
from the same codebase with **zero `#ifdef`s**.

---

## Why This Structure?

The original approach put everything in a single `os.c` with `#ifdef QEMU`
scattered throughout. This works for small labs but becomes unmaintainable as
complexity grows:

- Hard to read — platform logic mixed with application logic
- Error-prone — easy to forget an `#ifdef` or put code in the wrong branch
- Hard to add targets — every new board touches every file

The HAL/BSP pattern solves this by making the **build script** the only place
that knows about targets. Application code calls generic functions; the linker
connects them to the right hardware implementation.

---

## Folder Structure

```
0031InterruptHandler_refactored/
│
├── app/                        ← Application layer
│   └── os.c                    main(), app logic — NO hardware code
│
├── arch/                       ← CPU startup (assembly)
│   ├── boot.s                  Shared: vector table, exception handlers,
│   │                           PUT32/GET32, enable_irq, stack
│   ├── startup_qemu.s          QEMU reset_handler (copy vectors to 0x0)
│   └── startup_beagle.s        BeagleBone reset_handler (set VBAR register)
│
├── bsp/                        ← Board Support Package (hardware-specific C)
│   ├── qemu/
│   │   ├── uart_hw.c           PL011 UART (VersatilePB)
│   │   └── timer_hw.c          SP804 timer + PL190 VIC
│   └── beagle/
│       ├── uart_hw.c           AM335x UART0
│       └── timer_hw.c          DMTIMER2 + INTCPS interrupt controller
│
├── hal/                        ← Hardware Abstraction Layer (generic C)
│   ├── uart.h                  Declares uart_putc/getc (BSP) + os_write/read (HAL)
│   ├── uart.c                  Implements os_write(), os_read() using uart_putc/getc
│   ├── timer.h                 Declares timer_init (BSP) + timer_irq_handler (HAL)
│   ├── timer.c                 Implements timer_irq_handler() using timer_irq_clear
│   ├── interrupt.h             Declares enable_irq() (defined in boot.s)
│   └── types.h                 Declares PUT32/GET32 (defined in boot.s)
│
├── lib/                        ← Reusable libraries
│   ├── stdio.h                 Declares uart_putnum()
│   └── stdio.c                 Implements uart_putnum() using uart_putc
│
├── linker/                     ← Linker scripts (one per target)
│   ├── qemu.ld                 Code at 0x10000, 16KB stack
│   └── beagle.ld               RAM at 0x80000000, 256KB
│
├── out/                        ← Build output (one dir per target)
│   ├── qemu/                   program.elf, program.bin, *.o
│   └── beagle/                 program.elf, program.bin, *.o
│
├── build_and_run.sh            ← Build script — selects files per target
├── deploy_beagle.sh            ← Serial deploy to BeagleBone
├── register_lab.py             ← Registers VS Code tasks/launch configs
└── ymodem_send.py              ← Python ymodem handler (Windows deploy)
```

---

## Layer Responsibilities

### `app/` — Application Layer

Pure application logic. **Must not** contain hardware addresses, register
manipulation, or platform-specific code. Calls functions from `hal/` and `lib/`.

```
app/os.c calls:
  os_write()       → from hal/uart.h
  timer_init()     → from hal/timer.h
  enable_irq()     → from hal/interrupt.h
  uart_putnum()    → from lib/stdio.h
```

### `arch/` — CPU Startup (Assembly)

Three files, zero duplication:

| File | What | Shared? |
|------|------|---------|
| `boot.s` | Vector table, exception handlers, PUT32/GET32, enable_irq, stack | Yes — identical for all targets |
| `startup_qemu.s` | `reset_handler`: set stack, copy vectors to 0x0, call main | QEMU only |
| `startup_beagle.s` | `reset_handler`: set stack, set VBAR, call main | BeagleBone only |

**How they connect:** `boot.s` references `reset_handler` as an external symbol.
The linker resolves it from whichever `startup_*.s` was compiled. Same concept
as calling a function defined in another `.c` file.

**Where to put new assembly code:** Ask yourself — *does this change between
targets?* If no → `boot.s`. If yes → `startup_<target>.s` (or better yet,
write it in C inside `bsp/`).

### `bsp/` — Board Support Package

Platform-specific hardware implementations. Each target subfolder implements
the **same function signatures** declared in `hal/*.h`:

```
hal/uart.h declares:     void uart_putc(char c);
                          char uart_getc(void);

bsp/qemu/uart_hw.c   → implements for PL011 (VersatilePB)
bsp/beagle/uart_hw.c → implements for AM335x UART0
```

The build script compiles only one target's BSP:
```bash
# QEMU build:   bsp/qemu/*.c    is compiled
# Hardware build: bsp/beagle/*.c is compiled
```

**When to add files here:** Any code that touches hardware-specific registers
or addresses. If two targets need the same logic but different register
addresses, it belongs in `bsp/`.

### `hal/` — Hardware Abstraction Layer

Generic code that calls BSP functions. The HAL provides the API that `app/`
uses, hiding platform differences behind it.

```
app/os.c → calls os_write("hello")
  → hal/uart.c loops over string, calls uart_putc() for each char
    → bsp/qemu/uart_hw.c writes to PL011 UART_DR register
    OR
    → bsp/beagle/uart_hw.c writes to AM335x UART_THR register
```

**Headers in `hal/` serve dual purpose:**
- Declare BSP functions (implemented in `bsp/<target>/`)
- Declare HAL functions (implemented in `hal/*.c`)

### `lib/` — Reusable Libraries

Utility functions that build on top of `hal/`. Not hardware-specific, not
application-specific. Think of these as your "standard library":

- `stdio.c` — `uart_putnum()`, could grow into `printf()` later
- Future: `string.c`, `memory.c`, etc.

### `linker/` — Linker Scripts

One per target. Defines memory layout:

- `qemu.ld` — code at `0x10000` (VersatilePB convention)
- `beagle.ld` — code at `0x80000000` (DDR RAM on AM335x)

### `out/` — Build Output

Separate directories prevent cross-contamination between targets.
Each contains `.o` files, `program.elf` (for GDB), and `program.bin` (for deploy).

---

## How the Build Works

The build script is the **only place** that knows about targets:

```bash
./build_and_run.sh qemu         # Build + run in QEMU
./build_and_run.sh hardware     # Build for BeagleBone
./build_and_run.sh qemu_build_only  # Build only (used by VS Code F5)
```

For QEMU, it compiles:
```
arch/boot.s + arch/startup_qemu.s       ← assembly
bsp/qemu/uart_hw.c + bsp/qemu/timer_hw.c  ← platform-specific C
hal/uart.c + hal/timer.c                ← generic C
lib/stdio.c                             ← libraries
app/os.c                                ← application
```

For hardware, it swaps `arch/startup_beagle.s` and `bsp/beagle/*.c`.

No `-DQEMU` flag. No `#ifdef`. Just different source files.

---

## Call Flow

```
Power on / QEMU start
  → _start (boot.s)
    → vector_table
      → reset_handler (startup_qemu.s OR startup_beagle.s)
        → copies vectors / sets VBAR
        → main() (app/os.c)
          → os_write()        → hal/uart.c → bsp uart_putc()
          → timer_init()      → bsp timer_hw.c
          → enable_irq()      → boot.s
          → loop: uart_putnum() → lib/stdio.c → bsp uart_putc()

Timer fires → IRQ
  → irq_handler (boot.s) saves registers
    → timer_irq_handler() (hal/timer.c)
      → timer_irq_clear() (bsp timer_hw.c)
      → os_write("Tick\n") (hal/uart.c)
    → restores registers, returns from interrupt
```

---

## Adding a New Target

To add a third target (e.g., Raspberry Pi):

1. Create `arch/startup_raspi.s` — just the `reset_handler`
2. Create `bsp/raspi/uart_hw.c` — implement `uart_putc()`, `uart_getc()`
3. Create `bsp/raspi/timer_hw.c` — implement `timer_init()`, `timer_irq_clear()`
4. Create `linker/raspi.ld` — memory layout for the Pi
5. Add a target case in `build_and_run.sh`

No changes needed in `app/`, `hal/`, or `lib/`.

---

## Adding a New Peripheral

To add a new peripheral (e.g., GPIO):

1. Create `hal/gpio.h` — declare the generic API (`gpio_set()`, `gpio_clear()`)
2. Create `hal/gpio.c` — implement generic logic if any
3. Create `bsp/qemu/gpio_hw.c` — QEMU implementation
4. Create `bsp/beagle/gpio_hw.c` — BeagleBone implementation
5. Add the new `.c` files to `BSP_SRC` and `HAL_SRC` in `build_and_run.sh`
6. Use `#include "../hal/gpio.h"` from `app/os.c`

---

## Quick Reference

| I need to... | Put it in... |
|---|---|
| Write application logic | `app/` |
| Add a hardware driver | `bsp/<target>/` + declare in `hal/*.h` |
| Write platform-agnostic wrappers | `hal/*.c` |
| Add utility functions | `lib/` |
| Add startup assembly for a new board | `arch/startup_<target>.s` |
| Add shared assembly (any target) | `arch/boot.s` |
| Change memory layout | `linker/<target>.ld` |
| Change what gets compiled | `build_and_run.sh` |
