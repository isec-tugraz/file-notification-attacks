import numpy as np


def read_file(filename):
    with open(filename, "r") as f:
        return [float(line.strip()) for line in f]


watcher_data = read_file("op-watch")
accessor_data = read_file("op-accs")

if len(watcher_data) != len(accessor_data):
    raise ValueError("Both files must have the same number of lines.")

differences = [
    float(watcher) - float(accessor)
    for watcher, accessor in zip(watcher_data, accessor_data)
]

average = np.mean(differences) if len(differences) > 0 else 0.0
stddev = np.std(differences, ddof=1) if len(differences) > 1 else 0.0
stderr = stddev / np.sqrt(len(differences)) if len(differences) > 0 else 0.0

print(f"Average: {average * 1_000_000}us")
print(f"Standard Deviation: {stddev * 1_000_000}us")
print(f"Standard Error: {stderr * 1_000_000}us")
print(f"Minimum: {min(differences) * 1_000_000}us")
