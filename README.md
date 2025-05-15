# real-time-os-arm-kernel

![C](https://img.shields.io/badge/language-C-blue.svg)
![Platform](https://img.shields.io/badge/target-ARM%20Cortex--M3-red.svg)
![RTOS](https://img.shields.io/badge/RTOS-Custom%20Kernel-lightgrey.svg)
![Threads](https://img.shields.io/badge/Multitasking-Preemptive%20%7C%20Cooperative-green)
![Tools](https://img.shields.io/badge/Tools-Keil%20uVision%2C%20CMSIS-lightgrey)

A real-time operating system kernel built from scratch in C for ARM Cortex-M3. Implements preemptive scheduling, memory management, message passing, UART I/O, and system calls in a microkernel-style architecture.

---

## Features

| Subsystem             | Description                                             |
| --------------------- | ------------------------------------------------------- |
| **Task Management**   | Context switching, TCB tracking, task creation/deletion |
| **Scheduler**         | Round-robin scheduler with SysTick and PendSV handling  |
| **Memory Management** | Heap allocator with support for dynamic block reuse     |
| **IPC**               | Mailbox-based message passing between tasks             |
| **System Calls**      | SVC handler for safe kernel-user transitions            |
| **UART I/O**          | UART0 driver with interrupt-based input handling        |
| **Command Decoder**   | CLI parsing with registered command callbacks           |
| **Tree Structures**   | Internal tree usage for task/message storage            |
| **Debug Logging**     | UART-based runtime logging for traceability             |

---

## Platform and Tooling

* **Language:** C (CMSIS-style embedded systems style)
* **Target CPU:** ARM Cortex-M3 (simulated via Keil µVision)
* **Architecture:** Microkernel layout with user/kernel mode separation
* **Interrupts:** SysTick, PendSV, UART0 IRQs
* **Toolchain:** Keil µVision IDE (project files included)

---

## Project Structure

```
real-time-os-arm-kernel/
├── lab1/         # Kernel bootstrap, task init
├── lab2/         # Memory + IPC layer
├── lab3/         # Final scheduler, UART I/O, user programs
├── scripts/      # Build and project utilities
└── README.md     # This file
```

---

## Build & Test

Open `.uvprojx` project files with Keil µVision to compile and deploy. Run UART-based tests using serial terminal emulation. Each lab folder incrementally adds kernel features.

---

## Architecture Overview

```
+---------------------------+
|     User Task Layer       |
|  CLI apps, message senders |
+---------------------------+
            ↓
+---------------------------+
|    System Call Layer      |
|  SVC handler + validation |
+---------------------------+
            ↓
+---------------------------+
|      Scheduler + IPC      |
|   Round-robin, mailboxes  |
+---------------------------+
            ↓
+---------------------------+
|   Memory + UART Drivers   |
|  Heap mgmt, interrupt I/O |
+---------------------------+
            ↓
+---------------------------+
|     ARM Cortex-M3 SoC     |
+---------------------------+
```

---

## Summary

This project implements a full real-time kernel on bare-metal ARM, emphasizing safe context switching, memory safety, and task synchronization. Ideal for embedded systems, OS-level design, and real-time application experience.
