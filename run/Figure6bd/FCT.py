from collections import defaultdict
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pickle

with open('loading/config.pickle', "rb") as file:
    config = pickle.load(file)

folder_name, file_names, labels = config["folder_name"], config["file_names"], config["labels"]
colors, markers = config["colors"], config["markers"]

fig, ax = plt.subplots(nrows=1, figsize=(10, 6))
legends = []
for i in range(len(file_names)):
    flows = defaultdict(list)
    #with open('../../run/results/' + folder_name + file_names[i] + '.txt', 'r') as file:
    with open(folder_name + file_names[i], 'r') as file:
        for line in file.readlines():
            line = line.split()
            if len(line) <= 0:
                continue
            if line[0] == "FCT":
                flow_size = int(line[3])
                flow_FCT = float(line[4]) * 1E3
                flows[flow_size].append(flow_FCT)

    flow_sizes = [flow_size for (flow_size, flow_FCT) in flows.items()]
    flow_sizes.sort()
    FCTs = []
    for flow_size in flow_sizes:
        FCTs.append(np.percentile(flows[flow_size], 99))

    marker = None
    #linestyles = ['-', '--']
    line, = ax.plot(
        flow_sizes, FCTs,
        color=colors[i], label=labels[i], linestyle='-', marker=marker, linewidth=3
    )

    legends.append(line)

ax.set_xlim([0.5e2, 2e9])
ax.set_ylim([0.3e1, 2e7])
ax.set_xscale('log')
ax.set_yscale('log')

# Set x ticks
x_major = matplotlib.ticker.LogLocator(base=10.0, numticks=10)
ax.xaxis.set_major_locator(x_major)
x_minor = matplotlib.ticker.LogLocator(base=10.0, subs=np.arange(1.0, 10.0) * 0.1, numticks=10)
ax.xaxis.set_minor_locator(x_minor)
ax.xaxis.set_minor_formatter(matplotlib.ticker.NullFormatter())

# Set y ticks
y_major = matplotlib.ticker.LogLocator(base=10.0, numticks=10)
ax.yaxis.set_major_locator(y_major)
y_minor = matplotlib.ticker.LogLocator(base=10.0, subs=np.arange(1.0, 10.0) * 0.1, numticks=10)
ax.yaxis.set_minor_locator(y_minor)
ax.yaxis.set_minor_formatter(matplotlib.ticker.NullFormatter())

ax.set_ylabel("99%-ile FCT ($\mu s$)", fontsize=26)
ax.set_xlabel("Flow size (Bytes)", fontsize=26)

ax.tick_params(axis='x', labelsize=24)
ax.tick_params(axis='y', labelsize=24)

if config["legend"]:
    ax.legend(handles=legends, loc='upper center',ncol=6, bbox_to_anchor=(0.5, 1.15), handletextpad=0.07, fontsize=18, frameon=False)

plt.grid(ls='--')
plt.savefig(f'./figures/FCT.{config["suffix"]}', bbox_inches='tight')