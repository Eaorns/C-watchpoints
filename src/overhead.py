from data import data_wsl
from data import data_laptop
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np

items = 6

width = (1/(items))*.8

labels = ["$1$ variable",
          "$2^4$ variables",
          "$2^8$ variables",
          "$2^{12}$ variables",
          "$2^{16}$ variables",
          "$2^{20}$ variables"]

plt.figure(figsize=(9, 7))

for i, (sizes, writes) in enumerate(data_laptop.items()):

    d_overhead = [np.array([v[1][0] / v[0][0] for v in w]) for w in writes.values()]

    offset = .9*((1/items) * i - .5)
    color_offset = .7*(i/(items-1))

    plt.bar(np.arange(4) + offset, [np.mean(v) if v.size != 0 else 0 for v in d_overhead], 
            width=width, yerr=[np.std(v) if v.size != 0 else 0 for v in d_overhead], 
            capsize=3, color=(0, .95-.6*color_offset, .4+.2*color_offset), 
            label=f"{labels[i]}")


plt.xticks(range(4), ["$2^{16}$", "$2^{18}$", "$2^{20}$", "$2^{22}$"])
# plt.yscale('log')
plt.legend()
plt.xlabel("Number of writes", fontsize=12)
plt.ylabel("Overhead factor", fontsize=12)
plt.savefig("overhead-laptop.png")