# Private Set Intersection - Update (PSI-Update)

## Overview
This application implements the PSI Update protocol. It builds upon standard PSI by allowing the client to dynamically insert new elements into the server's Oblivious Map post-intersection, effectively updating the server's private set for future queries without exposing the updated elements to the host OS.

## Architecture
- **Untrusted Host (`App/`)**: Relays the standard batch ciphertexts and then subsequently relays the encrypted update payloads.
- **Trusted Enclave (`Enclave/`)**: Executes the standard intersection check and then processes the update payload by directly invoking `ecall_insert_element` to insert new elements into the `omap`.

## Execution Instructions
The execution follows the split-terminal architecture.

1. **Start the Server (Host):**
   ```bash
   ./algo_runner.sh 1
   ```
   This compiles the enclave and starts listening on port 8080.

2. **Start the Client (New Terminal):**
   ```bash
   make client
   ./client
   ```

## Expected Output
The server will log the processing of the batch queries and the subsequent insertions into the OMAP.
The client will output the standard network timings:
- `Online time taken: [X.X] s`
- `Total communication size: [X.X] KB`
