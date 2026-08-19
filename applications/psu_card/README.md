# Private Set Union - Cardinality (PSU-Card)

## Overview
This application implements a variation of Private Set Union known as PSU Cardinality (PSU-Card). It computes the union between the client and the server, but the client only learns the *total number of unique elements* (the cardinality of the union), rather than the elements themselves.

## Architecture
- **Untrusted Host (`App/`)**: Relays the batch queries to the enclave and transmits the final scalar union size back to the client over TCP.
- **Trusted Enclave (`Enclave/`)**: Computes the union obliviously. Instead of returning the union set, it returns only the securely calculated integer cardinality.

## Protocol Details

This application implements the Private Set Union Cardinality (PSU-CARD) protocol. It calculates the total number of unique items in the combined dataset, without revealing what the actual items are.

### Offline Setup Phase
The setup phase is exactly the same as the standard PSU protocol. The server generates encryption keys, encrypts its dataset, obliviously shuffles it, and builds a secure search tree.

### Online Phase
1. The client encrypts and sends its items to the server. 
2. Inside the secure enclave, the process starts with a running count equal to the size of the server's current dataset. 
3. The enclave decrypts each client item and obliviously searches for it in the secure tree. 
4. If the item is not found, it means it is a new unique item, so the enclave adds one to the running count. 
5. After checking all of the client's items, the enclave outputs this final number, representing the size of the union.

## Dependencies: OMap and O-Shuffle
To ensure strict data privacy and prevent access pattern leakage, this implementation relies heavily on two critical system-level components:
- **OMap (Oblivious Map)**: We utilize the EnigMap data structure for external-memory oblivious maps for secure enclaves. This prevents an adversary from inferring information by observing memory access patterns. EnigMap achieves search, insert, and delete operations in $O(\log^2 N)$ time. Original repository: [obliviouslabs/oram](https://github.com/obliviouslabs/oram) / [EnigMap Paper](https://eprint.iacr.org/2022/1083).
- **O-Shuffle (Oblivious Shuffling)**: We use the FlexWay O-Shuffle algorithm to randomly permute the dataset in a manner that hides the relationship between input and output positions. Original repository: [odslib/oblsort](https://github.com/odslib/oblsort).

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
The client will output the online timings:
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Total time taken: [X.X] s`
