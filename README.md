# PandOSsh - Phase 1

This repository contains the implementation of Phase 1 (The Queues Manager) for the PandOSsh operating system on uRISCV.

## Project Structure

* `phase1/`: Source code for PCB and ASL modules (`pcb.c`, `asl.c`) and the test file (`p1test.c`).
* `headers/`: Header files defining data structures and constants.
* `klog.c`: Kernel logging utility.
* `CMakeLists.txt`: Build configuration.

## Prerequisites

To build and run this project, you need:
1.  **uRISCV emulator**: Installed and configured.
2.  **GCC Toolchain for RISC-V**: `riscv64-unknown-elf-gcc` (or compatible).
3.  **CMake**: Version 3.25 or higher.

## How to Build

1.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

2.  Run CMake:
    ```bash
    cmake ..
    ```

3.  Compile the project:
    ```bash
    cmake --build .
    ```

This will generate the `Phase1.core.uriscv` and `Phase1.stab.uriscv` files required by the emulator.

## How to Run

1.  Ensure you have a valid machine configuration file (e.g., `config_machine.json`) pointing to the generated `.core.uriscv` and `.stab.uriscv` files.
2.  Launch the emulator:
    ```bash
    uriscv
    ```
3.  The test output will appear in Terminal 0. A successful execution ends with "System Halted".

## Authors

See the `AUTHOR` file for details.