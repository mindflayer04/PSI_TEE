# Private Set Intersection (PSI) Application

## Overview
This application implements the standard Private Set Intersection (PSI) protocol and Circuit PSI using the underlying parallel Oblivious Map (oMap) structures. It allows a client and a server to compute the intersection of their respective sets without revealing any elements that are not part of the intersection.

## Architecture
- **Untrusted Host (`App/`)**: Handles the network socket listening (defaulting to port 8080), client connection management, and SGX enclave initialization.
- **Trusted Enclave (`Enclave/`)**: Contains the secure oMap. It performs the intersection logic securely, ensuring the server's set remains entirely confidential, even from the untrusted OS.

## Execution Instructions
The execution uses a split-terminal architecture.

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
   The client will automatically connect, generate encrypted queries based on its hardcoded set, and send them to the server.

## Expected Output
The server will log connection events and online phase timings (computation vs. communication). 
The client will output standard network timings and explicitly state which elements are in the intersection:
- `Online time taken: [X.X] s`
- `Element [Y] is IN the intersection.`
