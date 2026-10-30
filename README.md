# Low-Level Systems, Binary Forensics & CPython Extensions

A collection of four low-level C and Linux systems engineering projects covering ELF reverse engineering and static forensic analysis, system call execution tracing (`strace`), high-performance CPython C API extension development, and POSIX async-signal-safe process control.

---

## Table of Contents

1. [Project 1: ELF Binary Forensics & Comparative Threat Assessment](https://www.google.com/search?q=%23project-1-elf-binary-forensics--comparative-threat-assessment)
2. [Project 2: Linux System Call Forensics & Execution Analysis](https://www.google.com/search?q=%23project-2-linux-system-call-forensics--execution-analysis)
3. [Project 3: CPython Extension for High-Performance Data Analysis](https://www.google.com/search?q=%23project-3-cpython-extension-for-high-performance-data-analysis)
4. [Project 4: Signal-Driven Linux Service Controller](https://www.google.com/search?q=%23project-4-signal-driven-linux-service-controller)

---

## Project 1: ELF Binary Forensics & Comparative Threat Assessment

### Overview & Source Architecture

This project analyzes structural and forensic variations introduced by source code evolution, compiler optimization levels, and symbol stripping between two versions of a directory synchronization utility (`data_sync` and `data_sync_v2`).

* **`data_sync.c` (Baseline - 3 functions):**
* `name_in_dir()`: Opens a directory with `opendir()`, iterates entries with `readdir()`, and matches names using `strcmp()`.
* `scan_directory()`: Traverses the source directory and reports entries absent in the target directory.
* `main()`: Validates CLI arguments and prints total sync candidate count.


* **`data_sync_v2.c` (Enhanced - 5 functions):**
* Retains directory traversal logic from `data_sync`.
* `needs_sync()`: Inspects file existence and size disparities using `stat()`.
* `checksum()`: Calculates a rolling checksum via `fopen()` and `fread()`.
* `write_log()`: Appends timestamped audit logs to `sync.log` using `time()`, `localtime()`, and `strftime()`.



### Compilation Matrix

| Binary | Compilation Command | Compiler Flags & Purpose |
| --- | --- | --- |
| `data_sync` | `gcc -Wall -Wextra -g -O0 -o data_sync data_sync.c` | **`-g`**: Includes DWARF debug info.<br>

<br>**`-O0`**: Disables optimizations for transparent assembly tracking. |
| `data_sync_v2` | `gcc -Wall -Wextra -O2 -s -o data_sync_v2 data_sync_v2.c` | **`-O2`**: Level-2 optimization (enables inline functions and Fortify checks).<br>

<br>**`-s`**: Strips symbol tables (`.symtab`, `.strtab`) and debug sections. |

---

### Comparative ELF Analysis Table

| Property / Feature | `data_sync` | `data_sync_v2` | Forensic Impact & Cause |
| --- | --- | --- | --- |
| **ELF Class & Machine** | ELF64, Little-Endian, x86-64 | ELF64, Little-Endian, x86-64 | Identical host target ABI. |
| **ELF Type** | DYN (Position-Independent Executable) | DYN (Position-Independent Executable) | Enabled by default (`-fPIE`); allows ASLR randomized base loading. |
| **Entry Point** | `0x1120` | `0x12b0` | Shifted due to `-O2` instruction scheduling and `.text` section reorganization. |
| **File Size** | 19,920 bytes | 14,472 bytes | `data_sync_v2` is ~27% smaller despite added code due to `-s` stripping DWARF `.debug_*` data. |
| **Section Headers** | 37 sections | 29 sections | Stripping removed `.symtab`, `.strtab`, and 6 `.debug_*` sections. |
| **Program Headers** | 13 segments | 13 segments | Runtime execution segment mapping remains identical. |
| **Symbol Table (`.symtab`)** | Present (45 entries) | Absent (`nm` reports "no symbols") | Stripped via `-s`. |
| **Dynamic Symbols (`.dynsym`)** | Present | Present | Mandatory for runtime dynamic linking; cannot be stripped. |
| **Imported Functions** | `opendir`, `readdir`, `closedir`, `strcmp`, `printf`, `fprintf`, `perror` | `opendir`, `readdir`, `closedir`, `stat`, `fopen`, `fread`, `fclose`, `time`, `localtime`, `strftime`, `perror`, `__printf_chk`, `__fprintf_chk`, `__snprintf_chk`, `__stack_chk_fail` | Added file metadata, checksumming, and logging routines. `-O2` replaced `printf`/`fprintf` with fortified `_chk` variants and inlined `strcmp`. |
| **GLIBC Versions** | `GLIBC_2.2.5`, `GLIBC_2.34` | `GLIBC_2.2.5`, `GLIBC_2.3.4`, `GLIBC_2.4`, `GLIBC_2.33`, `GLIBC_2.34` | Additional system headers (`sys/stat.h`, `time.h`) introduced higher GLIBC symbol versioning requirements. |

---

### Sections vs. Segments & Startup Analysis

```
       LINKING VIEW (Sections)               LOADING / RUNTIME VIEW (Segments)
+------------------------------------+      +------------------------------------+
| ELF Header / Section Header Table  |      | Program Header Table (13 headers)  |
+------------------------------------+      +------------------------------------+
| .text (Executable Code)            | ---> | LOAD Segment (Read-Execute: R E)   |
| .rodata (String Literals)          | ---> | LOAD Segment (Read-Only: R)        |
| .data / .bss (Global Variables)    | ---> | LOAD Segment (Read-Write: RW)      |
+------------------------------------+      +------------------------------------+
| .symtab / .debug_* (Stripped by -s)|      | DYNAMIC Segment (Loaded into RAM)  |
+------------------------------------+      +------------------------------------+

```

* **Sections vs. Segments:**
* **Sections:** Named, typed data blocks (`.text`, `.rodata`) used for compilation, static linking, and debugging (`readelf -S`). Can be stripped without breaking execution.
* **Segments:** Program headers (`readelf -l`) defining memory permissions (`R`, `RE`, `RW`) for kernel page mapping via `mmap()`.


* **Stripped Symbol Recovery (`main` Location):**
1. `_start` is located at the ELF entry point (`0x12b0`).
2. Disassembly reveals `lea -0x8f(%rip), %rdi` evaluating to `0x1240`, followed immediately by `call __libc_start_main@got`.
3. This proves that `main()` is located at address **`0x1240`** despite `.symtab` stripping.



---

### Static Forensic Assessment Report

```
================================================================================
                    STATIC BINARY FORENSIC ASSESSMENT REPORT
================================================================================
TARGETS: data_sync (Unoptimized/Debug) vs. data_sync_v2 (Optimized/Stripped)
ENVIRONMENT: Ubuntu 24.04 LTS | GCC 13.3 | x86_64
METHODOLOGY: Static Analysis Only (file, readelf, nm, objdump, strings)
--------------------------------------------------------------------------------

1. EXECUTIVE SUMMARY:
   Both binaries are 64-bit Position-Independent Executables (PIE) dynamically
   linked against libc.so.6. Comparative inspection demonstrates evolutionary 
   feature expansion from basic directory listing to file content metadata 
   validation, rolling checksumming, and file-backed log output.

2. TECHNICAL FINDINGS:
   - Security Protections: Both binaries leverage full RELRO (BIND_NOW), 
     Non-Executable Stack (GNU_STACK RW), and PIE (ASLR support).
   - Hardening Artifacts: compilation of data_sync_v2 with -O2 automatically
     injected glibc Fortify Source wrappers (__printf_chk, __snprintf_chk) and
     GCC canary stack protection (__stack_chk_fail).
   - Capability Indicators:
     * data_sync: Pure name matching (opendir, readdir, strcmp).
     * data_sync_v2: File attribute verification (stat), raw block IO (fopen,
       fread), and temporal log generation (time, localtime, strftime -> sync.log).

3. FORENSIC CONCLUSION:
   Static imports confirm data_sync_v2 expands capability without introducing
   network socket creation, privilege escalation, process execution (execve),
   or destructive file unlinking routines.
================================================================================

```

---

## Project 2: Linux System Call Forensics & Execution Analysis

### Overview & Program Logic

This module analyzes system call interaction patterns during successful and failed file backup executions using `strace`. The program creates a target backup directory, logs operation status to `backup/backup.log`, forks a child process to copy a target source file into the backup directory, and waits for process completion.

### System Call Execution Summary

| System Call | Relevant Arguments | Return Value / Status | Program Execution Role |
| --- | --- | --- | --- |
| `openat()` | `AT_FDCWD`, `"backup/backup.log"`, `O_WRONLY|O_CREAT|O_APPEND`, `0644` | `= 3` | Opens log file descriptor. |
| `openat()` | `AT_FDCWD`, `"src.txt"`, `O_RDONLY` | `= 4` (Success)<br>

<br>`= -1 ENOENT` (Failed) | Opens source file. Missing source file triggers execution failure. |
| `openat()` | `AT_FDCWD`, `"backup/src.txt"`, `O_WRONLY|O_CREAT|O_TRUNC`, `0644` | `= 5` | Opens/truncates destination backup file descriptor. |
| `read()` | `fd=4`, `buf`, `count=1024` | `= 13` (Data read)<br>

<br>`= 0` (EOF) | Reads source file contents in chunks. `0` breaks the copy loop. |
| `write()` | `fd=5`, `"hello backup\n"`, `len=13` | `= 13` | Writes payload to backup file descriptor. |
| `write()` | `fd=3`, `"[1] Backup directory ready\n"`, `len=27` | `= 27` | Appends formatted entry to `backup.log`. |
| `write()` | `fd=2`, `"open missing.txt: No such file..."`, `len=44` | `= 44` | Emits stderr error message upon copy failure. |
| `mkdir()` | `"backup"`, `0755` | `= 0` (Success)<br>

<br>`= -1 EEXIST` (Failed run) | Creates directory. `EEXIST` is handled as non-fatal. |
| `clone()` | `flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD` | `= 43099` (Child PID) | Glibc wrapper for `fork()` to spawn the copy worker. |
| `wait4()` | `pid=43099`, `status`, `options=0` | `= 43099` (`WEXITSTATUS == 0` or `1`) | Synchronizes parent with child completion. |
| `exit_group()` | `status=0` or `1` | Does not return | Terminates process and all threads. |

---

### Execution Analysis & Trace Phenomena

```
PARENT PROCESS (PID 1000)                CHILD PROCESS (PID 43099)               KERNEL / OS
-------------------------                -------------------------               -----------
openat("backup.log") = 3
clone() = 43099 ────────────────────────► [Spawns Child Process]
wait4(43099, <unfinished ...>) [BLOCKED]
                                         openat("src.txt") = 4
                                         openat("backup/src.txt") = 5
                                         read(4) = 13 -> write(5) = 13
                                         read(4) = 0 (EOF)
                                         exit_group(0) ────────────────────────► Kernel sends SIGCHLD
wait4(...) resumed = 43099 ◄───────────────────────────────────────────────────── Unblocks Parent
write(3, "[3] Copy succeeded\n")

```

1. **Process Creation & Race Interleaving:**
* After `clone()`, parent and child processes run concurrently.
* `strace` logs child system calls (e.g., `set_robust_list()`) interleaved **before** the parent's `wait4(..., <unfinished ...>)` entry due to CPU scheduling order.


2. **Failure Analysis (Missing File Test):**
* On missing source files, `openat()` returns `-1 ENOENT`.
* The child skips destination file creation, writes error text to `stderr` (`fd=2`), and terminates with `exit_group(1)`.
* The parent receives `WEXITSTATUS(s) == 1` via `wait4()`, logs `"[3] Copy FAILED"`, and exits with code `1`.


3. **Reliability Enhancements:**
* **Atomic Copy Writes:** Copying directly to a destination with `O_TRUNC` risks destroying old backup copies if the write fails mid-operation. Replacing direct writes with a temporary file write followed by `rename()` guarantees atomic updates.
* **Distinct Exit Codes:** Returning specific exit statuses (e.g., `2` for `ENOENT`, `3` for permissions) allows the parent process to handle distinct failure reasons programmatically.



---

## Project 3: CPython Extension for High-Performance Data Analysis

### Architecture Overview

This module implements a CPython C API extension (`c_python_pybaseline`) to accelerate statistical processing, outlier detection, and min-max normalization over large numeric sequences.

```
       +-------------------------------------------------------+
       |               Python Application Layer                |
       +-------------------------------------------------------+
                                   |
                import c_python_pybaseline (dlopen)
                                   v
       +-------------------------------------------------------+
       |             CPython C Extension Interface             |
       |  - PyArg_ParseTuple() : Unbox Python arguments        |
       |  - PySequence_Fast()  : Direct array access           |
       +-------------------------------------------------------+
                                   |
                                   v
       +-------------------------------------------------------+
       |             Native C Computational Kernel           |
       |  - Contiguous double* buffer memory allocation        |
       |  - Pass 1: Mean, Min, Max, Outliers Count             |
       |  - Pass 2: Population Variance                        |
       |  - Pass 3: Min-Max Normalization Loop                 |
       +-------------------------------------------------------+
                                   |
                                   v
       +-------------------------------------------------------+
       |          Py_BuildValue() / PyList_SET_ITEM            |
       |          Returns Result Dict to Python Runtime        |
       +-------------------------------------------------------+

```

---

### Compilation & Integration Setup

#### `setup.py`

```python
from setuptools import setup, Extension

c_module = Extension(
    'c_python_pybaseline',
    sources=['fast_stats.c'],
    extra_compile_args=['-O3', '-Wall', '-Wextra']
)

setup(
    name='c_python_pybaseline',
    version='1.0',
    description='CPython acceleration extension for data analytics',
    ext_modules=[c_module]
)

```

#### Build Commands

```bash
# Clean previous build artifacts
rm -f *.so

# Build C Extension in-place
python3 setup.py build_ext --inplace

```

---

### Benchmark Results

* **Environment:** Ubuntu 24.04 LTS, Python 3.12, GCC 13.3 (`-O3`)
* **Operations:** Mean, Variance, Min, Max, Threshold Outlier Counting, and Min-Max Normalization

| Dataset Size ($N$) | Pure Python Avg Time (s) | C Extension Avg Time (s) | Speedup Factor ($\frac{T_{py}}{T_c}$) |
| --- | --- | --- | --- |
| **1,000** | $0.000993$ | $0.000183$ | **5.42x** |
| **100,000** | $0.028969$ | $0.005401$ | **5.36x** |
| **1,000,000** | $0.298142$ | $0.060042$ | **4.97x** |

---

### Technical Performance Report

1. **CPython C API Mechanics:**
* **Unboxing:** `PyArg_ParseTuple()` extracts raw Python objects. `PySequence_Fast_ITEMS()` converts generic sequences into native contiguous double arrays, eliminating per-element object attribute lookups and dynamic dispatch overhead.
* **Reference Management:** `PyList_SET_ITEM()` steals references to constructed `PyFloat` objects, avoiding memory leaks during list construction.


2. **Compilation Pipeline:**
* `setup.py` compiles `fast_stats.c` into positional-independent code linked with `Python.h`.
* Generates `c_python_pybaseline.cpython-312-x86_64-linux-gnu.so`.
* Calling `import c_python_pybaseline` invokes `dlopen()`, executing `PyInit_c_python_pybaseline()` to register the module's method table.


3. **Bottleneck Analysis:**
* While computation loops execute at native speeds, speedup plateaus at **~5x**.
* **The Bottleneck:** Converting $1,000,000$ double values back into distinct CPython `PyFloatObject` instances via `PyFloat_FromDouble()` requires individual heap allocations, introducing high runtime memory management overhead.



---

## Project 4: Signal-Driven Linux Service Controller

### Overview & Architecture

A robust POSIX service controller that changes operational state (`NORMAL` vs `MAINTENANCE`) and handles status reports or termination commands via signal handlers.

```c
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#define REPORT_INTERVAL 5

/* Global flags shared between signal handler and main loop */
volatile sig_atomic_t status_flag = 0;
volatile sig_atomic_t toggle_flag = 0;
volatile sig_atomic_t stop_flag = 0;

void handle_signal(int sig)
{
	if (sig == SIGUSR1)
		status_flag = 1;
	else if (sig == SIGUSR2)
		toggle_flag = 1;
	else if (sig == SIGINT)
		stop_flag = 1;
}

void handle_term(int sig)
{
	const char msg[] = "\n[Server Controller] EMERGENCY TERMINATION\n";
	ssize_t ret;

	(void)sig;
	ret = write(STDOUT_FILENO, msg, sizeof(msg) - 1);
	(void)ret;
	_exit(1);
}

```

---

### Signal Registration & Async-Safety

```
[External OS Signal] ───► Asynchronous Interrupt ───► handle_signal(sig)
                                                            │
                                        Sets atomic flag (e.g., status_flag = 1)
                                                            │
                                                            v
[Main Loop Execution] ◄─── Synchronous Check ◄─── process_flags(&state)
         │
         ├───► Executes printf() safely outside signal context
         └───► Flushes pending status / state updates

```

1. **Signal Registration:**
`setup_signals()` uses `sigaction()` with empty signal masks (`sigemptyset`) to register custom handlers (`handle_signal` for `SIGUSR1`, `SIGUSR2`, `SIGINT`; `handle_term` for `SIGTERM`).
2. **Async-Signal Safety:**
Functions like `printf()` or `malloc()` are non-reentrant and unsafe inside signal handlers. To prevent deadlocks and memory corruption, `handle_signal()` only mutates global flags declared as `volatile sig_atomic_t`.
3. **Volatile `sig_atomic_t` Semantics:**
* `sig_atomic_t`: Guarantees single-instruction, atomic read/write operations.
* `volatile`: Forces the compiler to fetch values directly from memory on every read rather than caching values in CPU registers.


4. **State Control & Shutdown Differences:**
* **`SIGUSR1` / `SIGUSR2`:** Sets `status_flag` or `toggle_flag`. `process_flags()` toggles the `maintenance` state (`*maintenance = !(*maintenance)`) synchronously inside the main loop.
* **`SIGINT` (Graceful Shutdown):** Sets `stop_flag = 1`. The main loop terminates naturally, allowing standard cleanup and exit routines to execute.
* **`SIGTERM` (Emergency Shutdown):** Invokes `handle_term()`, which uses the async-signal-safe system calls `write()` and `_exit(1)` to terminate the process immediately without running deferred handlers or cleanup routines.



---

### Terminal Demonstration Log

```bash
# Compilation
gcc -Wall -Wextra -o server_controller server_controller.c

# Start Server Controller
./server_controller
[Server Controller] Started. PID: 45210
[Server Controller] Service running normally...

# Send SIGUSR1 (Status Check)
kill -SIGUSR1 45210
[Server Controller] STATUS: NORMAL | PID: 45210

# Send SIGUSR2 (Toggle State to Maintenance)
kill -SIGUSR2 45210
[Server Controller] Mode changed to MAINTENANCE
[Server Controller] In maintenance mode...

# Send SIGINT (Graceful Shutdown)
kill -SIGINT 45210
[Server Controller] SIGINT received. Shutting down...
[Server Controller] Cleanup complete. Goodbye.

# Send SIGTERM (Emergency Termination)
kill -SIGTERM 45210
[Server Controller] EMERGENCY TERMINATION

```
