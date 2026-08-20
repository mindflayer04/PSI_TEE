import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

os.makedirs('perf_plots', exist_ok=True)

df = pd.read_csv('perf_data.csv')

protocols = df['Protocol'].unique()
server_sizes = df['Server Set Size'].unique()
client_sizes = df['Client Set Size'].unique()

# 1. Online Time Plots
for s_size in server_sizes:
    subset = df[df['Server Set Size'] == s_size]
    
    plt.figure(figsize=(10, 6))
    x = np.arange(len(protocols))
    width = 0.25
    
    for i, c_size in enumerate(client_sizes):
        c_subset = subset[subset['Client Set Size'] == c_size]
        times = []
        for p in protocols:
            val = c_subset[c_subset['Protocol'] == p]['Online Total Time (ms)'].values
            times.append(val[0] if len(val)>0 else 0)
        
        plt.bar(x + (i - 1) * width, times, width, label=f'Client Size {c_size}')
    
    plt.ylabel('Online Total Time (ms)')
    plt.title(f'Online Total Time vs Protocol (Server Size: {s_size})')
    plt.xticks(x, protocols, rotation=15)
    plt.legend()
    plt.tight_layout()
    plt.savefig(f'perf_plots/online_time_{s_size.replace("^", "")}.png')
    plt.close()

# 2. Comm Size Plot (using first server size, since it's independent)
subset = df[df['Server Set Size'] == server_sizes[0]]
plt.figure(figsize=(10, 6))
x = np.arange(len(protocols))
width = 0.25
for i, c_size in enumerate(client_sizes):
    c_subset = subset[subset['Client Set Size'] == c_size]
    sizes = []
    for p in protocols:
        val = c_subset[c_subset['Protocol'] == p]['Comm. Data Size'].values
        sizes.append(val[0] if len(val)>0 else 0)
    plt.bar(x + (i - 1) * width, sizes, width, label=f'Client Size {c_size}')

plt.ylabel('Communication Data Size (KB)')
plt.title('Communication Data Size vs Protocol')
plt.xticks(x, protocols, rotation=15)
plt.legend()
plt.tight_layout()
plt.savefig('perf_plots/comm_size.png')
plt.close()

print("Plots generated successfully in perf_plots/")
