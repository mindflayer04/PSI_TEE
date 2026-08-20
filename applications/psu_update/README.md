# Updatable Unbalanced Private Set Union (UPSU)

## Overview
This repository implements the Updatable PSU  protocol, where only the server can delete some of its old entries. In addition to securely computing the union of the client and server datasets, this variant introduces server-side data management. By maintaining a privacy-preserving log database, the server can securely track and delete outdated or expired entries without compromising the underlying data structure.

## Architecture
- **Untrusted Host (`App/`)**: Relays the standard batch ciphertexts for the union computation and subsequently forwards the encrypted payload of new elements to be inserted.
- **Trusted Enclave (`Enclave/`)**: Processes offline updates via `ecall_insert_batch` and executes the online union computation, which seamlessly inserts new unique client elements into the secure Oblivious Map.

## Protocol Details

This repository implements the Updatable Unbalanced Private Set Union (UPSU) protocol. It extends the standard PSU protocol by adding the ability for the server to remove old or expired items from its dataset without needing to rebuild the entire secure search tree.

### Update Phase
When the server needs to delete items:
1. It first encrypts and obliviously shuffles the list of items it wants to remove. 
2. The secure enclave then takes this list and securely deletes each item from the main search tree one by one. 

### Online Phase
After updates are finished, the online phase works exactly the same as the standard PSU protocol:
1. The client sends its encrypted items to the server.
2. The enclave safely merges them by checking for their existence in the secure tree and adding them if they are unique, forming a new combined list. At the end, the server receives the desired result only.

## Default Update Configuration
Before listening for client connections, the server executes an offline update phase where it explicitly batch inserts elements to simulate dynamic dataset growth. By default, it generates a batch of 64 dummy elements (`"dummy0"` to `"dummy63"`) and profiles the insertion timings. 
During the online phase, the server also updates the dataset dynamically with any new unique elements provided by the client.
- To change the offline elements being inserted, modify the `insert_elements` vector in `App/TrustedLibrary/Libcxx.cpp`. To change the online updates, modify the `client_set` loop in `client.cpp`.

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
   ./client 2> client.log
   ```
   *(Note: We run the client with `2> client.log` instead of just `./client` as otherwise the `cerr` statements would also start to get printed to the console.)*

## Expected Output
The server will log the processing of the batch queries and the subsequent insertions into the OMAP.
The client will output the standard network timings:
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Union size: [N]`
- `Total time taken: [X.X] s`
