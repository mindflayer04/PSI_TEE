# Unbalanced Private Set Union (PSU) Protocol

## Overview
This repository implements the Unbalanced Private Set Union (PSU) protocol utilizing the parallel Oblivious Map. In PSU, the server  computes the union of their sets without leaking any additional information.

## Architecture
- **Untrusted Host (`App/`)**: Manages the socket connections and orchestrates the transmission of batch ciphertexts representing the client's set.
- **Trusted Enclave (`Enclave/`)**: Executes `ecall_check_union_batch` to securely compute the union between the client's ciphertexts and the server's securely stored Oblivious Map. 

## Protocol Details

This repository implements the Unbalanced Private Set Union (PSU) protocol, which allows two parties to compute their combined datasets without the client seeing the server's data.

### Offline Setup Phase
1. The server generates encryption keys, encrypts its own dataset, obliviously shuffles it, and builds a secure search tree. 
2. The client also sets up a key pair, though for a standard union, the final result stays on the server.

### Online Phase
1. The client encrypts its entire dataset and sends it to the server. 
2. Inside the secure enclave, the server starts a new list using its own original dataset as the base. 
3. It then decrypts each of the client's items and obliviously searches for it in the secure tree. 
4. If an item is not found (meaning it is unique to the client), the enclave adds it to the combined list. If it is already there, it gets discarded.
5. Finally, the enclave outputs this merged list and the server receives the final union only.

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
