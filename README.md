# Doubly-efficient Unbalanced  PSI and PSU from Trusted Hardware with Offline Pre-processing 

## Abstract
This repository contains the reference implementation for our **Unbalanced Private Set Intersection (PSI)** protocol and its variants. Built on Intel SGX and optimized with a parallel Oblivious Map (OMap) data structure, this repository also includes the complete implementation of our **Unbalanced Private Set Union (PSU)** protocol and its different variants.

> [!WARNING]
> **Hardware & Environment Warning**
> 
> To ensure maximum compatibility across reviewer environments and standard hardware, the build scripts in this artifact default to **SGX Simulation Mode (`SGX_MODE=SIM`)**. 
> Furthermore, to prevent Out-Of-Memory (OOM) crashes on machines with limited RAM, the dataset sizes inside the client and server benchmarks have been strictly fixed. 

---

## Protocol Overview

This repository implements six core protocols utilizing a preprocessing based offline-online model combined with Trusted Execution Environments (e.g., Intel SGX):
- **PSI (Private Set Intersection)**: Computes the intersection between the client and the server datasets.
- **PSI-CARD (Private Set Intersection Cardinality)**: Computes only the cardinality (size) of the intersection and the client receives it only.
- **UPSI (Updatable Private Set Intersection)**: An updatable PSI variant supporting dynamic additions/deletions on the server side.
- **PSU (Private Set Union)**: It computes the union of both datasets securely and the server receives it.
- **PSU-CARD**: Computes the size of the set union and the server receives it only.
- **UPSU (Updatable Private Set Union)**: An updatable variant of unbalanced PSU protocol.

These protocols rely heavily on two critical cryptographic dependencies:
- **OMap (Oblivious Map)**: Prevents access pattern leakage by utilizing the EnigMap data structure. Repository: [obliviouslabs/oram](https://github.com/obliviouslabs/oram) / [EnigMap Paper](https://eprint.iacr.org/2022/1083).
- **O-Shuffle (Oblivious Shuffling)**: Hides the relationship between input and output positions via the FlexWay O-Shuffle Algorithm. Repository: [odslib/oblsort](https://github.com/odslib/oblsort).

For in-depth details on each protocol, refer to the READMEs located inside the respective application folders.

## Directory Structure

Explore the dedicated READMEs for each sub-application for deeper technical insights:

```text
.
├── BLAKE3/              # Fast cryptographic hash function source code
├── applications/        # SGX Enclave Applications (PSI, PSU, Benchmarks)
│   ├── benchmark/       # Primary application for the AE workflow ([README](applications/benchmark/README.md))
│   ├── omap/            # Core oMap functionality testing ([README](applications/omap/README.md))
│   ├── psi/             # Standard Private Set Intersection ([README](applications/psi/README.md))
│   ├── psi_card/        # PSI Cardinality ([README](applications/psi_card/README.md))
│   ├── psi_update/      # PSI Update ([README](applications/psi_update/README.md))
│   ├── psu/             # Private Set Union ([README](applications/psu/README.md))
│   ├── psu_card/        # PSU Cardinality ([README](applications/psu_card/README.md))
│   └── psu_update/      # PSU Update ([README](applications/psu_update/README.md))
├── cmake/               # CMake configuration modules
├── omap/                # Core C++ library for Oblivious Data Structures
├── tests/               # Unit testing modules
└── tools/
    └── docker/          # Reproducible Docker environment build files
```

---

## NDSS Artifact Evaluation Workflow (Split-Terminal)

To evaluate the artifact, we employ a split-terminal strategy. Reviewers will use one terminal to act as the SGX Host (Server), and a second terminal to act as the querying Client.

### Step 1: Building and Running the Docker Container
First, build and enter the isolated Docker environment which contains all the necessary dependencies (CMake, Ninja, Intel SGX SDK).

```bash
# Build the Docker image
docker build -t cppbuilder:latest ./tools/docker/cppbuilder

# Run and enter the container interactively
docker run -it --rm --name psi_eval -p 8080:8080 -v $PWD:/builder -u $(id -u) cppbuilder
```

### Step 2: Launching the SGX Server (Terminal 1)
Inside the container, navigate to the benchmark application directory and use the automated build script to compile the enclave and start the server host.

```bash
cd applications/benchmark/
./algo_runner.sh 1
```

Once executed, the server will successfully build the enclave and present an **interactive menu**. 
Enter the number corresponding to the protocol you wish to benchmark (e.g., `1` for standard PSI). The server will then generate the RSA keys, initialize the Oblivious Map in the enclave, and state that it is listening on port 8080.

### Step 3: Compiling and Running the Client (Terminal 2)
Leave Terminal 1 running. Open a *new* terminal on your host machine, and enter the active Docker container.

```bash
# Enter the running container
docker exec -it psi_eval /bin/bash

# Navigate to the benchmark application
cd applications/benchmark/

# Compile the client executable
make client

# Run the client
./client
```

The client will present a matching interactive menu. **Select the identical protocol number** that you chose in Step 2. The client will connect to the server, exchange ciphertexts, and complete the protocol execution.

### Step 4: Interpreting the Metrics
Once the protocol finishes, the client will terminate and print the final metrics directly to your console (Terminal 2).

Reviewers should look for the following exact phrases in the standard output to verify the performance claims presented in the paper:
- **`Online time taken: [X.X] s`**
- **`Total time taken: [X.X] s`**
- **`Total communication size: [X.X] KB`**

These metrics correspond directly to the execution time and total communication bounds evaluated in our experimental results.

---

## Advanced Configuration

If you wish to run the artifact on a high-performance machine or explore larger dataset boundaries, you can modify the core execution variables located inside the `algo_runner.sh` script for each application.

### Changing SGX Modes (HW vs SIM)
By default, the artifact runs in Simulation Mode (`SIM`). If you have compatible Intel hardware and the SGX drivers installed, you can switch to Hardware Mode:
1. Open `algo_runner.sh`.
2. Change `SGX_MODE=SIM` to `SGX_MODE=HW`.

### Changing Dataset Sizes
By default, the experiments use synthetically generated sets of fixed sizes. If you wish to change the size of the evaluated datasets, you can do so by manipulating the variables directly in the source code:
- **Client Set Size**: Open `client.cpp` in the respective application's directory and modify the `client_set` variable (which defines the client's input array).
- **Server Set Size**: Open `App/TrustedLibrary/Libcxx.cpp` in the respective application's directory and modify the `server_set` variable.

### Disk I/O Settings
The `DISK_IO` parameter dictates whether the Oblivious Map aggressively swaps memory to the external disk to circumvent enclave memory limits.
- Set `DISK_IO=0` in `algo_runner.sh` to keep all data securely inside the enclave RAM. (Provides faster performance but strictly limits maximum dataset sizes before causing OOM).
- Set `DISK_IO=1` (Default) to enable external disk swapping for large datasets.

### Enclave RAM Size (Heap Size)
The maximum amount of RAM the enclave is allowed to utilize is controlled dynamically by the `algo_runner.sh` script using the `<HeapMaxSize>` tag. 
To increase the Enclave RAM allocation for larger datasets, modify the `MIN_ENCLAVE_SIZE` and `MAX_ENCLAVE_SIZE` bounds in `algo_runner.sh`.

### Server Backend Size (Handling Bigger Elements)
The current configuration optimizes memory footprints based on the standard protocol requirements. If you wish to run the application with **bigger elements** (i.e., larger cryptographic payloads or keys), you must correspondingly increase the server's backend storage size. To do this, modify the `size_t BackendSize` variable located in the `Enclave/TrustedLibrary/omap_test.cpp` file of the respective application directory.

> [!TIP]
> **Backend Size Logic:**
> When calculating the required backend storage for custom element sizes, you can take a **conservative approach of 64 bytes per element**. Therefore, if you are computing an intersection for $N$ custom elements, you should configure the `BackendSize` variable to accommodate at least $N \times 64$ bytes of storage. This buffer safely accounts for metadata and cipher overhead.

## Experimental Results

Below are the benchmark results of the different protocols (PSI, PSI Cardinality, PSI Update, PSU, PSU Cardinality, PSU Update) evaluated across varying Server Set Sizes ($2^{24}$, $2^{26}$, $2^{28}$) and Client Set Sizes ($2^8$, $2^9$, $2^{10}$).

### Online Total Time
The following graphs illustrate the Online Total Time (in milliseconds) for each protocol across the different Server Set Sizes.

![Online Time (Server Size 2^24)](./perf_plots/online_time_224.png)
![Online Time (Server Size 2^26)](./perf_plots/online_time_226.png)
![Online Time (Server Size 2^28)](./perf_plots/online_time_228.png)

### Communication Data Size
The communication data size (in KB) relies purely on the Client Set Size and the underlying protocol semantics (independent of the Server Set Size). 

![Communication Data Size](./perf_plots/comm_size.png)
