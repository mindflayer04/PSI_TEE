# Unbalanced Private Set Union Cardinality (PSU-CA)

## Overview
This repository implements a variant of Unbalnced Private Set Union protocol known as PSU Cardinality (PSU-CA). It computes the union between the client and the server, but the server only learns the *total number of unique elements* (the cardinality of the union), rather than the elements themselves.

## Architecture
- **Untrusted Host (`App/`)**: Relays the batch queries to the enclave and transmits the final scalar union size back to the client over TCP.
- **Trusted Enclave (`Enclave/`)**: Computes the union obliviously. Instead of returning the union set, it returns only the securely calculated integer cardinality.

## Protocol Details

This repository implements the Unbalanced Private Set Union Cardinality (PSU-CA) protocol. It calculates the total number of unique items in the combined dataset, without revealing any extra information.

### Offline Setup Phase
The setup phase is exactly the same as our unbalanced PSU protocol. The server generates encryption keys, encrypts its dataset, obliviously shuffles it, and builds a secure search tree.

### Online Phase
1. The client encrypts and sends its items to the server. 
2. Inside the secure enclave, the process starts with a running count equal to the size of the server's current dataset. 
3. The enclave decrypts each client item and obliviously searches for it in the secure tree. 
4. If the item is not found, it means it is a new unique item, so the enclave adds one to the running count. 
5. After checking all of the client's items, the enclave outputs this final number to the server only, representing the size of the union.

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
The client will output the online timings:
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Total time taken: [X.X] s`
