# Private Set Union - Update (PSU-Update)

## Overview
This application implements the PSU Update protocol. In addition to computing the union of the client and server sets, this protocol allows the client to dynamically append elements to the server's Oblivious Map (oMap) during the union operation, updating the server's private set for subsequent queries.

## Architecture
- **Untrusted Host (`App/`)**: Relays the standard batch ciphertexts for the union computation and subsequently forwards the encrypted payload of new elements to be inserted.
- **Trusted Enclave (`Enclave/`)**: Executes the union computation and directly calls `ecall_insert_element` to insert the new client-provided elements into the secure Oblivious Map.

## Protocol Details

This application implements the Updatable Private Set Union (PSU-UPD) protocol. It extends the standard PSU protocol by adding the ability for the server to remove old or expired items from its dataset without needing to rebuild the entire secure search tree.

### Update Phase
When the server needs to delete items:
1. It first encrypts and obliviously shuffles the list of items it wants to remove. 
2. The secure enclave then takes this list and securely deletes each item from the main search tree one by one. 

### Online Phase
After updates are finished, the online phase works exactly the same as the standard PSU protocol:
1. The client sends its encrypted items to the server.
2. The enclave safely merges them by checking for their existence in the secure tree and adding them if they are unique, forming a new combined list.

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
- `Total encryption time for [N] elements: [X.X] s`
- `Sent set size: [N]`
- `Online time taken: [X.X] s`
- `Union size: [N]`
- `Total time taken: [X.X] s`
