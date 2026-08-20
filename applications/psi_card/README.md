# Unbalanced Private Set Intersection Cardinality (PSI-CA)

## Overview
This repository implements a variation of the unbalanced PSI protocol known as PSI Cardinality (PSI-CA). In this protocol, the client only learns the *total size* (cardinality) of the intersection, rather than the specific elements that matched and the server receives nothing.

## Architecture
- **Untrusted Host (`App/`)**: Manages the socket connections and orchestrates the batch transmission of ciphertexts from the client.
- **Trusted Enclave (`Enclave/`)**: The secure environment securely counts the intersections using the oblivious map and only returns the final scalar count to the host to be transmitted back to the client.

## Protocol Details

This repository implements the Unbalanced Private Set Intersection Cardinality (PSI-CA) protocol. It allows the client to learn the total number of matching items (the cardinality of the intersection), but it does not reveal which specific items matched.

### Offline Setup Phase
The setup phase is identical to the standard PSI protocol. The server generates encryption keys, hashes, encrypts, and obliviously shuffles its items, and then builds an oblivious search tree.

### Online Query Phase
1. The client encrypts its items and sends them to the server.
2. Inside the secure enclave, the server starts with a running count initially set to 0.
3. The enclave decrypts each client item and obliviously searches for it in the secure tree. 
4. Every time it finds a match, it adds one to the running count. 
5. After checking every item, the enclave encrypts the final count using the client's public key and sends it back. 
6. The client decrypts this single message to learn the size of the intersection.

## Dependencies: OMap and O-Shuffle
To ensure strict data privacy and prevent access pattern leakage, this implementation relies heavily on two critical system-level components:
- **OMap (Oblivious Map)**: We utilize the EnigMap data structure for external-memory oblivious maps for secure enclaves. This prevents an adversary from inferring information by observing memory access patterns. EnigMap achieves search, insert, and delete operations in $\tilde{O}(\log^2 N_{s})$ time. Original repository: [obliviouslabs/oram](https://github.com/obliviouslabs/oram) / [EnigMap Paper](https://eprint.iacr.org/2022/1083).
- **O-Shuffle (Oblivious Shuffling)**: We use the FlexWay O-Shuffle algorithm to randomly permute the dataset in a manner that hides the relationship between input and output positions. Original repository: [odslib/oblsort](https://github.com/odslib/oblsort).

## Execution Instructions
The execution follows the split-terminal architecture.

1. **Start the Server (Host):**
   ```bash
   ./algo_runner.sh 1
   ```
   This compiles the enclave and starts the server. You will be prompted to enter the server set size as a power of 2 (e.g., enter `24` for $2^{24}$). The server then starts listening on port 8080.

2. **Start the Client (New Terminal):**
   ```bash
   make client
   ./client 2> client.log
   ```
   *(Note: We run the client with `2> client.log` instead of just `./client` as otherwise the `cerr` statements would also start to get printed to the console.)*

## Expected Output
The client will log the online phase time and the final intersection count:
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Intersection count: [N]`
- `Total time taken: [X.X] s`
