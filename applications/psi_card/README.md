# Private Set Intersection - Cardinality (PSI-Card)

## Overview
This application implements a variation of the standard PSI protocol known as PSI Cardinality (PSI-Card). In this protocol, the client and server engage in an oblivious intersection, but the client only learns the *total size* (cardinality) of the intersection, rather than the specific elements that matched.

## Architecture
- **Untrusted Host (`App/`)**: Manages the socket connections and orchestrates the batch transmission of ciphertexts from the client.
- **Trusted Enclave (`Enclave/`)**: The secure environment securely counts the intersections using the oblivious map and only returns the final scalar count to the host to be transmitted back to the client.

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
The client will log the online phase time and the final intersection count:
- `Online time taken: [X.X] s`
- `Intersection count: [N]`
