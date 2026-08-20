# Updatable Unbalanced Private Set Intersection (UPSI) Protocol

## Overview
This repository implements the **one sided** Unbalanced Updatable PSI protocol. The server can dynamically insert new elements into, or delete existing elements from, the preprocessed data structure by updating the underlying Oblivious Map. The client can then execute the PSI protocol against this newly updated database. This capability is particularly advantageous for password monitoring systems, where clients must privately check their credentials against an ever-evolving list of breached passwords.

## Architecture
- **Untrusted Host (`App/`)**: Relays the standard batch ciphertexts and then subsequently relays the encrypted update payloads.
- **Trusted Enclave (`Enclave/`)**: Executes the standard intersection check and then processes the update payload by directly invoking `ecall_insert_element` to insert new elements into the `omap`.

## Protocol Details

This repository implements the Updatable Unbalanced Private Set Intersection (UPSI) protocol. It extends the standard PSI protocol by allowing the server to dynamically add or remove elements from its dataset over the time, without having to rebuild the entire OMap search tree from scratch.

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

## Default Update Configuration
By default, the server explicitly inserts new elements into its dataset during the update phase. The default configuration initializes an initial server set of size $2^{24}$ (elements $0$ to $2^{24}-1$) and an update set of size $2^8$. These $2^8$ new elements are generated to be completely unique (i.e., starting from $2^{24}$ onwards) so that they trigger fresh insertions into the OMap.
- To change these elements, open `App/TrustedLibrary/Libcxx.cpp` and modify the loop responsible for generating the `new_elements` vector.

## Dependencies: OMap and O-Shuffle
To ensure strict data privacy and prevent access pattern leakage, this implementation relies heavily on two critical system-level components:
- **OMap (Oblivious Map)**: We utilize the EnigMap data structure for external-memory oblivious maps for secure enclaves. This prevents an adversary from inferring information by observing memory access patterns. EnigMap achieves search, insert, and delete operations in $\bar{O}(\log^2 N_{s})$ time. Original repository: [obliviouslabs/oram](https://github.com/obliviouslabs/oram) / [EnigMap Paper](https://eprint.iacr.org/2022/1083).
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
   ./client
   ```

## Expected Output
The server will log the processing of the batch queries and the subsequent insertions into the OMAP.
The client will output the standard network timings:
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Element [Y] is IN the intersection.` (or `NOT in the intersection.`)
- `Total time taken: [X.X] s`
