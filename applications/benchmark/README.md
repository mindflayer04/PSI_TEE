# Benchmark Application

## Overview
This application exists as an easy, unified way to benchmark the actual 6 protocols (PSI, PSU, and their variants) without needing to configure and run each one individually. It provides an interactive environment where users can quickly test the performance of these implementations under fixed conditions.

For in-depth details on how each of the 6 protocols works, please refer to their dedicated READMEs:
- [Standard PSI](../psi/README.md)
- [PSI Cardinality](../psi_card/README.md)
- [Updatable PSI](../psi_update/README.md)
- [Standard PSU](../psu/README.md)
- [PSU Cardinality](../psu_card/README.md)
- [Updatable PSU](../psu_update/README.md)

---

## Supported Protocols
The benchmark suite supports the following 6 protocols selectable via an interactive menu:

- **PSI (Unbalanced  Private Set Intersection)**: Computes the intersection between the client and the server datasets.
- **PSI-CA (Unbalanced  Private Set Intersection Cardinality)**: Computes only the cardinality (size) of the intersection and the client receives it only.
- **UPSI (Updatable Unbalanced  Private Set Intersection)**: An updatable PSI variant supporting dynamic additions/deletions on the server side.
- **PSU (Unbalanced  Private Set Union)**: It computes the union of both datasets securely and the server receives it.
- **PSU-CA (Unbalanced Private Set Union Cardinality)**: Computes the size of the set union and the server receives it only.
- **UPSU (Unbalanced  Updatable Private Set Union)**: An updatable variant of unbalanced PSU protocol.

---

## Architecture & Optimizations

Like all SGX applications in this repository, the benchmark is split into untrusted host and trusted enclave components:
- **Untrusted Host (`App/`)**: Manages TCP socket networking, RSA public key distribution, client ciphertext reception, offline dataset initialization, and benchmarking orchestration.
- **Trusted Enclave (`Enclave/`)**: Performs secure RSA-2048 OAEP decryption, oblivious map lookups, batch insertion/deletion (`ecall_insert_batch`, `ecall_delete_batch`), and secure union/intersection evaluations.

### Key Performance & Sizing Features:
- **Large-Scale Memory Support**: Configured with up to 190 GB Enclave Heap Size (`0x2f80000000`), 32 TCS thread slots, a 150 GB in-enclave cache, and a multi-terabyte external memory backend.
- **Persistent Dataset across Runs**: The server dataset and Enclave OMap are constructed once at server startup. The server remains in an interactive loop allowing multiple protocols or repeated runs without re-initializing the entire map.
- **Batch Processing**: Enclave operations leverage `GLOBAL_BATCH_SIZE` (1,000,000 elements) for fast batch insertion and deletion routines.

---

## Execution Instructions

The benchmark execution follows a client-server model.

### 1. Start the Server (Host)
Navigate to the benchmark directory and execute the runner script:
```bash
cd applications/benchmark/
./algo_runner.sh 1
```

1. **Enter Server Dataset Size**: When prompted, enter the dataset size as a power of 2 (e.g., enter `24` for $2^{24} \approx 16.7\text{M}$ elements):
   ```text
   Enter the server set size (as a power of 2, e.g., 24 for 2^24): 24
   ```
2. **Select Protocol**: Once the RSA keys are generated and the Enclave OMap is initialized, select the protocol number from the menu:
   ```text
   Select Protocol to Benchmark:
   0. Exit
   1. PSI
   2. PSI Cardinality
   3. PSI Update
   4. PSU
   5. PSU Cardinality
   6. PSU Update
   Enter choice (0-6): 1
   ```
3. The server will run any offline update benchmarks (if Protocols 3 or 6 were selected) and begin listening on port `8080`.

### 2. Start the Client (New Terminal / Client Node)
Open a new terminal or connect from a client machine (update `SERVER_IP` in [client.cpp](file:///home/Jitu/PSI_aashirwad/PSI_TEE/applications/benchmark/client.cpp) if running over a network):
```bash
cd applications/benchmark/
make client
./client
```

Select the **exact same protocol number** that was selected on the server:
```text
Select Protocol to Benchmark:
0. Exit
1. PSI
2. PSI Cardinality
3. PSI Update
4. PSU
5. PSU Cardinality
6. PSU Update
Enter choice (0-6): 1
```

---

## Expected Output & Metric Interpretation

### Client-Side Output
Upon completion, the client displays cryptographic timings, communication overhead, and protocol-specific outcomes:

- **Common Metrics**:
  ```text
  Connected to server. Receiving public key...
  Public key received successfully.
  Total encryption time for [N] elements: [X.X] s
  Sent set size: [N]
  Online time taken: [X.X] s
  Total time taken: [X.X] s
  Total communication size: [X.X] KB
  ```

- **Protocol-Specific Outcomes**:
  - **PSI / PSI Update (Options 1 & 3)**:
    ```text
    Element [X] is IN the intersection.
    Element [Y] is NOT in the intersection.
    ```
  - **PSI Cardinality (Option 2)**:
    ```text
    Intersection count: [N]
    ```
  - **PSU Cardinality (Option 5)**:
    ```text
    Union size: [N]
    ```
  - **PSU / PSU Update (Options 4 & 6)**:
    ```text
    PSU protocol completed successfully.
    ```

### Server-Side Output
The server logs enclave creation timings, offline update metrics, and online latency breakdown:
- **Map Creation**: `[Host] Map created successfully in enclave. Time taken: [X.X] ms`
- **Offline Updates (Protocols 3 & 6)**:
  ```text
  [Host] Total insertion timings: [X.X] ms
  [Host] Insertion timings per element: [X.X] ms/element
  [Host] Total deletion timings: [X.X] ms
  [Host] Deletion timings per element: [X.X] ms/element
  ```
- **Online Phase Breakdown**:
  ```text
  [Host] Online Phase Total Time: [X.X] ms
  [Host]   - Communication Time: [X.X] ms
  [Host]   - Computation Time: [X.X] ms
  ```
