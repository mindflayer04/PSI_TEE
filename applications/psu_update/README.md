# Private Set Union - Update (PSU-Update)

## Overview
This application implements the PSU Update protocol. In addition to computing the union of the client and server sets, this protocol allows the client to dynamically append elements to the server's Oblivious Map (oMap) during the union operation, updating the server's private set for subsequent queries.

## Architecture
- **Untrusted Host (`App/`)**: Relays the standard batch ciphertexts for the union computation and subsequently forwards the encrypted payload of new elements to be inserted.
- **Trusted Enclave (`Enclave/`)**: Executes the union computation and directly calls `ecall_insert_element` to insert the new client-provided elements into the secure Oblivious Map.

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
- `Union size: [N]`
