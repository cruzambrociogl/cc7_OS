# 001 Multiprogramming — Project Plan

## Goal
Build a bare-metal multiprogramming system for the BeagleBone Black (AM335x, Cortex-A8).
Three separate binaries (OS, P1, P2) share the CPU via round-robin scheduling driven by a timer interrupt.

## Memory Map

| Binary | Load Address  | Size  | Purpose                        |
|--------|---------------|-------|--------------------------------|
| OS     | `0x82000000`  | 64KB  | Hardware init, scheduler, IRQ  |
| P1     | `0x82100000`  | 64KB  | User process: prints 0-9       |
| P2     | `0x82200000`  | 64KB  | User process: prints a-z       |

## Project Structure

```
001Multiprogramming/
├── arch/                  # Assembly: vectors, startup, context switch
│   ├── boot.s             # Vector table + IRQ handler
│   ├── startup_beagle.s   # OS reset: .bss clear, stacks, VBAR, bl main
│   └── startup_proc.s     # P1/P2 minimal startup
├── hal/                   # Hardware Abstraction Layer
│   ├── types.h            # PUT32/GET32 declarations
│   ├── uart.h / uart.c    # os_write, os_read
│   ├── timer.h / timer.c  # timer_irq_handler (calls scheduler)
│   └── interrupt.h
├── bsp/beagle/            # Board Support Package (AM335x specific)
│   ├── uart_hw.c          # UART0 putc/getc
│   └── timer_hw.c         # DMTimer2 + INTC + Watchdog
├── lib/                   # Shared library (linked into all 3 binaries)
│   ├── stdio.h / stdio.c  # PRINT() implementation
│   ├── string.h / string.c
├── os/                    # OS-specific code
│   ├── main.c             # Init everything, start P1
│   ├── pcb.h / pcb.c      # PCB struct, init
│   └── scheduler.c        # Round-robin logic
├── P1/main.c              # User process 1
├── P2/main.c              # User process 2
├── linker/
│   ├── os.ld              # @ 0x82000000
│   ├── p1.ld              # @ 0x82100000
│   └── p2.ld              # @ 0x82200000
├── build_and_run.sh       # Compiles 3 binaries
└── out/                   # Build output
```

## How It Works

1. U-Boot loads `os.bin`, `p1.bin`, `p2.bin` to their fixed addresses
2. `go 0x82000000` → OS boots, disables watchdog, inits UART/Timer/INTC
3. OS initializes PCBs with entry points and stack pointers for P1 and P2
4. OS enables IRQs, then jumps to P1
5. P1 runs (prints digits), until DMTimer2 fires (~1 second)
6. IRQ handler saves P1's full context (R0-R12, SP, LR, PC) into its PCB
7. Scheduler picks P2, IRQ handler loads P2's context from its PCB
8. P2 runs (prints letters), until next timer tick
9. Repeat forever — round-robin between P1 and P2

## U-Boot Load Sequence

```
loady 0x82000000    → send os.bin
loady 0x82100000    → send p1.bin
loady 0x82200000    → send p2.bin
go 0x82000000
```

---

## Implementation Steps

### Step 1 — Project structure ✅
Created directory skeleton following the HAL/BSP pattern from 0031InterruptHandler_refactored.

### Step 2 — Linker scripts ✅
Created 3 linker scripts placing each binary at its own address:
- `linker/os.ld` — ORIGIN = 0x82000000, exports `__bss_start__`/`__bss_end__`
- `linker/p1.ld` — ORIGIN = 0x82100000
- `linker/p2.ld` — ORIGIN = 0x82200000

### Step 3 — .bss clearing in startup assembly ✅
Created `arch/startup_beagle.s` based on the 0031 version but with 3 key additions:
1. **Dual stack setup** — SVC stack at `0x82012000`, IRQ stack at `0x82014000` (the original only had one stack)
2. **Mode switching** — explicitly sets SVC and IRQ modes to configure both stacks
3. **`.bss` clearing loop** — zeros memory from `__bss_start__` to `__bss_end__` so globals (PCB array) start at zero

### Step 4 — Watchdog disable ✅
Added `wdt_disable()` to `bsp/beagle/timer_hw.c`. Called at the start of `timer_init()`. Writes `0xAAAA` then `0x5555` to WDT1_WSPR with posted-write waits to prevent board reset after ~60 seconds.

### Step 5 — PCB structure ✅
Created `os/pcb.h` with PCB struct (pid, state, regs[13], sp, lr, pc, cpsr) and constants (READY=0, RUNNING=1).

### Step 6 — PCB initialization ✅
Created `os/pcb.c` with `pcb_init()`. Sets P1 entry=0x82100000 sp=0x82112000, P2 entry=0x82200000 sp=0x82212000, all regs=0, cpsr=0x13 (SVC mode, IRQs enabled).

### Step 7 — IRQ handler with context switch ✅
Created `arch/boot.s` with full context-switch IRQ handler:
1. Saves R0-R12 to `saved_regs[]`, return address to `saved_lr`
2. Switches to SVC mode to read banked SP/LR, stores to `saved_svc_sp`/`saved_svc_lr`
3. Calls C handler `timer_irq_handler()`
4. Restores next process's SVC SP/LR (via mode switch)
5. Restores R0-R12 and returns with `movs pc, lr`

### Step 8 — Round-robin scheduler ✅
Implemented inside `hal/timer.c` → `timer_irq_handler()`:
- Clears timer IRQ
- Saves global buffers → `pcb[current]`
- Swaps `current_process` between 1 and 2
- Loads `pcb[next]` → global buffers

### Step 9 — Initial context switch (OS → P1) ✅
Added `start_first_process` in `arch/boot.s`:
- Takes PCB pointer in R0
- Loads SP and PC from PCB
- Enables IRQs
- Jumps to process entry point

### Step 10 — User process code ✅
Created `P1/main.c` (prints 0-9) and `P2/main.c` (prints a-z). Both use `PRINT()` from shared lib.
Created `arch/startup_proc.s` as minimal entry point for P1/P2.

### Step 11 — Build script ✅
Created `build_and_run.sh` that compiles 3 separate binaries:
- OS: arch + hal + bsp + lib + os → `out/os.bin`
- P1: startup_proc + boot (for PUT32/GET32) + uart + stdio + P1 → `out/p1.bin`
- P2: same structure → `out/p2.bin`

### Step 12 — Load instructions ✅
Build script prints U-Boot load sequence at the end. Also created supporting HAL/BSP files
(types.h, uart.h, uart.c, uart_hw.c) reused from 0031InterruptHandler_refactored.
Upgraded lib/stdio to full PRINT() with %d, %c, %s support.

---

## Status: COMPLETE ✅

All 12 steps implemented and tested on BeagleBone Black hardware.
P1 prints 0-9, timer fires, context switches to P2 which prints a-z, then back to P1.

## Bugs Fixed During Development

| Bug | Cause | Fix |
|-----|-------|-----|
| Hang on `pcb_init()` | `-mfpu=neon -mfloat-abi=hard` made GCC emit NEON instructions (`vmov.i32`, `vst1.32`) for struct init, but FPU/NEON not enabled in startup → undefined instruction exception | Changed CFLAGS to `-mfloat-abi=soft` |
| `saved_regs` misaligned | `.data` section lacked `.align 2`, placing `saved_regs` at non-4-byte-aligned address → potential data abort on `str`/`ldr` | Added `.align 2` before data declarations in `boot.s` |
| Duplicate `ldr sp` in `start_first_process` | Two `ldr sp` instructions, first with wrong offset | Removed duplicate, kept correct offset (60) |
