# Private Set Intersection (PSI) Application

## Overview
This repository implements the Unbalanced Private Set Intersection (PSI) protocol using the underlying parallel Oblivious Map (OMap) data structure. It allows a client and a server to compute the intersection of their respective sets without revealing any elements that are not part of the intersection.

## Architecture
- **Untrusted Host (`App/`)**: Handles the network socket listening (defaulting to port 8080), client connection management, and SGX enclave initialization.
- **Trusted Enclave (`Enclave/`)**: Contains the secure oMap. It performs the intersection logic securely, ensuring the server's set remains entirely confidential, even from the untrusted OS.

## Protocol Details

This application implements the Private Set Intersection (PSI) protocol, which allows a client and server to compute the intersection of their private datasets without revealing any additional information. The protocol follows a preprocessing based offline-online computation model to maximize efficiency for unbalanced datasets (e.g., the server's dataset is significantly larger than the client).

### Offline Setup Phase
In the offline phase, the server prepares its dataset to allow for faster online queries:
1. **Key Generation**: The server's secure enclave generates a public/private key pair. The public key is shared with the client.
2. **Data Preparation**: The server hashes, encrypts, and obliviously shuffles its dataset. 
3. **Oblivious Search Tree Construction**: The encrypted and shuffled dataset is structured into an oblivious search tree.

The client also prepares for the online phase by hashing its dataset.

### Online Query Phase
1. The client encrypts its hashed items using the server's public key and sends them to the server.
2. Inside the secure enclave, the server decrypts each client item and obliviously searches for it within the secure tree. 
3. The result of each search is encrypted using the client's public key and sent back.
4. The client decrypts the result to determine if each item was in the intersection.

## Dependencies: OMap and O-Shuffle
To ensure strict data privacy and prevent access pattern leakage, this implementation relies heavily on two critical system-level components:
- **OMap (Oblivious Map)**: We utilize the EnigMap data structure for external-memory oblivious maps for secure enclaves. This prevents an adversary from inferring information by observing memory access patterns. EnigMap achieves search, insert, and delete operations in $\bar{O}(\log^2 N_{s})$ time. Original repository: [obliviouslabs/oram](https://github.com/obliviouslabs/oram) / [EnigMap Paper](https://eprint.iacr.org/2022/1083).
- **O-Shuffle (Oblivious Shuffling)**: We use the FlexWay O-Shuffle algorithm to randomly permute the dataset in a manner that hides the relationship between input and output positions. Original repository: [odslib/oblsort](https://github.com/odslib/oblsort).

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
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Element [Y] is IN the intersection.` (or `NOT in the intersection.`)
- `Total time taken: [X.X] s`
