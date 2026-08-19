# oMap (Oblivious Map) Example Application

## Overview
This application serves as a standalone test environment for the core Oblivious Map (oMap) library. It is designed to evaluate the functionality and performance of the highly parallel oMap structure in external memory scenarios independently from specific cryptographic protocols like PSI or PSU.

## Architecture
The application adheres to the standard SGX execution model:
- **Untrusted Host (`App/`)**: Manages external memory file I/O, paging, and the initialization of the secure enclave.
- **Trusted Enclave (`Enclave/`)**: Implements the oblivious data structures (Circuit ORAM, recursive ORAM) and processes all load-balanced batch queries securely.

## Execution Instructions
You can execute the oMap application manually via its build script:

```bash
# Execute in simulation mode (default)
./algo_runner.sh 1
```

If you wish to test the asynchronous disk I/O performance across varying enclave sizes, you can execute `./algo_runner.sh` without the `1` argument, which will output the logs to `.out` files.

## Expected Output
When executed, the host application will output metrics corresponding to the initialization time of the Oblivious Map and the latency for sequential and batch queries.

Example output:
- `Init_Time: [X] ms`
- `Latency of batch access: [X] ms`
