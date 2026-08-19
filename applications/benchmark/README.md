# Benchmark Application

## Overview
This application serves as the primary evaluation suite for benchmarking the performance of the Oblivious Map (oMap) and the various Private Set Intersection (PSI) and Private Set Union (PSU) protocols. It allows users to quickly test multiple protocols under fixed conditions.

## Architecture
Like all SGX applications in this repository, it is split into two components:
- **Untrusted Host (`App/`)**: Handles network connections, client interactions, and launches the enclave. It runs the primary C++ entry point (`App.cpp`) and the `ActualMain()` host logic.
- **Trusted Enclave (`Enclave/`)**: Executes the core cryptographic operations and manages the Oblivious Map in secure memory. 

## Execution Instructions
You can execute this benchmark manually using the automated build script, which defaults to SGX Simulation Mode (`SGX_MODE=SIM`) and dynamically configures the required enclave memory limits.

1. **Start the Server (Host):**
   ```bash
   ./algo_runner.sh 1
   ```
   Select the desired protocol from the interactive menu.

2. **Start the Client (New Terminal):**
   ```bash
   make client
   ./client
   ```
   Select the *matching* protocol from the client's interactive menu.

## Expected Output
The client application will print the final performance metrics to standard output, specifically:
- `Online time taken: [X.X] s`
- `Total communication size: [X.X] KB`

These logs are critical for verifying the performance claims of the evaluated protocols.
