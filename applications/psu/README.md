# Private Set Union (PSU)

## Overview
This application implements the Private Set Union (PSU) protocol utilizing the parallel Oblivious Map. In PSU, the client and server jointly compute the union of their sets. The client securely learns the complete union without the server learning which elements were contributed by the client.

## Architecture
- **Untrusted Host (`App/`)**: Manages the socket connections and orchestrates the transmission of batch ciphertexts representing the client's set.
- **Trusted Enclave (`Enclave/`)**: Executes `ecall_check_union_batch` to securely compute the union between the client's ciphertexts and the server's securely stored Oblivious Map. 

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
The client will output the online timings and the final calculated size of the union:
- `Online time taken: [X.X] s`
- `Union size: [N]`
