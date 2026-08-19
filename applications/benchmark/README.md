# Benchmark Application

## Overview
This application exists as an easy, unified way to benchmark the actual 6 protocols (PSI, PSU, and their variants) without needing to configure and run each one individually. It provides an interactive environment where users can quickly test the performance of these implementations under fixed conditions.

For in-depth details on how each of the 6 protocols works, please refer to their dedicated READMEs:
- [Standard PSI](../psi/README.md)
- [PSI Cardinality](../psi_card/README.md)
- [Updatable PSI](../psi_update/README.md)
- [Standard PSU](../psu/README.md)
- [PSU Cardinality](../psu_card/README.md)
- [Updatable PSU](../psu_update/README.md)

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
   An interactive menu will open up on the server side. You must select which of the 6 protocols you wish to benchmark by entering the corresponding number.

2. **Start the Client (New Terminal):**
   ```bash
   make client
   ./client
   ```
   An interactive menu will also open up on the client side. You must select the *exact same* protocol that you selected on the server to begin the benchmark.

## Expected Output
Depending on the specific protocol chosen from the interactive menu, the client application will calculate and print the final performance metrics to standard output. A complete execution will output phrases identical to the following:
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Total time taken: [X.X] s`
- `Total communication size: [X.X] KB`

If you are running a Cardinality or Update protocol, you may also see additional contextual output, such as:
- `Intersection count: [N]`
- `Union size: [N]`

These logs are critical for verifying the performance claims of the evaluated protocols.
