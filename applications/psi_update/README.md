# Private Set Intersection - Update (PSI-Update)

## Overview
This application implements the PSI Update protocol. It builds upon standard PSI by allowing the client to dynamically insert new elements into the server's Oblivious Map post-intersection, effectively updating the server's private set for future queries without exposing the updated elements to the host OS.

## Architecture
- **Untrusted Host (`App/`)**: Relays the standard batch ciphertexts and then subsequently relays the encrypted update payloads.
- **Trusted Enclave (`Enclave/`)**: Executes the standard intersection check and then processes the update payload by directly invoking `ecall_insert_element` to insert new elements into the `omap`.

## Protocol Details

This application implements the Updatable Private Set Intersection (PSI-UPD) protocol. It extends the standard PSI protocol by allowing the server to dynamically add or remove elements from its dataset over time, without having to rebuild the entire secure search tree from scratch.

### Setup and Update Phases
When the server needs to update its dataset (additions or deletions):
1. **Update Preparation**: The server receives a list of new items to add and old items to remove. It hashes, encrypts, and obliviously shuffles these update lists.
2. **Tree Modification**: The secure enclave uses oblivious addition and deletion functions to modify the existing oblivious search tree. 

### Online Query Phase
The online query phase remains exactly the same as the standard PSI protocol:
1. The client encrypts its hashed items using the server's public key and sends them to the server.
2. Inside the secure enclave, the server decrypts each client item and obliviously searches for it within the secure tree. 
3. The result of each search is encrypted using the client's public key and sent back.
4. The client decrypts the result to determine if each item was in the intersection.

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
The server will log the processing of the batch queries and the subsequent insertions into the OMAP.
The client will output the standard network timings:
- `Online time taken: [X.X] s`
- `Total communication size: [X.X] KB`
