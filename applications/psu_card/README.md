# Private Set Union - Cardinality (PSU-Card)

## Overview
This application implements a variation of Private Set Union known as PSU Cardinality (PSU-Card). It computes the union between the client and the server, but the client only learns the *total number of unique elements* (the cardinality of the union), rather than the elements themselves.

## Architecture
- **Untrusted Host (`App/`)**: Relays the batch queries to the enclave and transmits the final scalar union size back to the client over TCP.
- **Trusted Enclave (`Enclave/`)**: Computes the union obliviously. Instead of returning the union set, it returns only the securely calculated integer cardinality.

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
