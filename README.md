# Real-Time Operating System Kernel — ARM Cortex-M (ECE 350)

> ⚙️ This custom RTOS kernel, implemented for a simulated ARM Cortex-M3 platform, achieves performance comparable to early Intel x86 CPUs such as the **Intel 80386DX (1985)** — the first 32-bit processor in the x86 line. While the 80386 ran at 16–33 MHz with hardware-supported multitasking and memory segmentation, this project recreates key OS functionality — including thread scheduling, inter-process communication, and memory management — in software, from scratch. It demonstrates the architectural complexity required to support real-time, multi-threaded applications on embedded processors.

---

## 🧠 Overview
This project implements a complete real-time operating system (RTOS) kernel in C, targeting the ARM Cortex-M architecture (simulated via the Keil µVision IDE). It builds a foundational, microkernel-style system from the ground up, encompassing memory, scheduling, communication, and hardware abstraction layers.

By the end of the project, the kernel supports dynamic task creation, cooperative and preemptive multitasking, inter-process messaging, and device I/O via UART — all under strict timing and reliability constraints typical of real-time systems.

---

## 🔧 Features Implemented

| Subsystem | Description |
|----------|-------------|
| **Task Management** | Creation, deletion, and context-switching for multiple tasks. Maintains task control blocks (TCBs) and enforces privilege separation. |
| **Scheduler** | Cooperative and preemptive round-robin scheduler using SysTick and PendSV interrupts. Efficient context switching. |
| **Memory Management** | Custom dynamic memory allocator supporting variable-sized allocation and deallocation with fragmentation handling. |
| **Inter-Process Communication (IPC)** | Mailbox system supporting blocking/non-blocking message sending and receiving. |
| **System Calls & Trap Handling** | User programs invoke kernel services via SVC (Supervisor Call) exceptions. Includes argument marshalling and privilege-level enforcement. |
| **UART Integration** | Device driver for UART0 to support basic I/O, user interaction, and keyboard command decoding. Supports interrupt-driven input. |
| **Command Interface** | Kernel Command Decoder (KCD) for parsing and dispatching CLI commands from user tasks. |
| **Tree-Based Structures** | Efficient internal message/mailbox/task storage via custom balanced-tree data structures. |
| **Logging** | Basic logging and debugging support with serial output, useful for testing, timing, and traceability. |

---

## 🛠 Technical Stack
- **Language:** C (CMSIS-compliant, hardware-adjacent style)
- **Target:** ARM Cortex-M3 (via Keil µVision)
- **Architecture:** Microkernel design with preemptive scheduler
- **Interrupts:** SysTick for timer; PendSV for context switching; UART0 for I/O
- **Tools:** Keil µVision IDE, semihosted debugging, project scripts

---

## 📂 Repository Structure

```
.
├── lab1/                   # Initial kernel boot, memory setup, task management basics
├── lab2/                   # Expanded with message passing, syscall trap handling
├── lab3/                   # Final full kernel: scheduler, UART I/O, user programs
├── scripts/               # Project utilities
```
Each lab builds on the previous to form a complete, progressively evolved kernel.

---

## ✅ Testing & Validation
- **Preemptive task switching** tested using timer-driven interrupts
- **Message passing** validated with multiple concurrent sender/receiver threads
- **Memory allocator** verified with edge-case allocations and fragmentation scenarios
- **UART and CLI** tested interactively and under load with multiple simultaneous inputs

Test coverage was ensured by embedded test programs, manual scenarios, and system-level integration tests across each lab stage.

---

## 💡 Educational Significance
This kernel replicates core behaviors of real-world embedded operating systems and illustrates the full control stack from user task to hardware. It is a foundational RTOS implementation that demonstrates understanding of:
- Context switching and CPU state management
- Safe preemptive multitasking
- Device driver and interrupt integration
- Systems programming in constrained environments

The resulting system is robust, efficient, and realistic — suitable for further extension into a production-grade microcontroller OS.
