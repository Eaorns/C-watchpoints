from data import data_wsl
from data import data_laptop
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np

# x_axis = [k for k in data_wsl[1].keys()]
# # x_axis = [1,2,3,4]

# for sizes, writes in data_wsl.items():
#     d_clean = [np.array([i[0][1] for i in w]) for w in writes.values()]
#     d_wpts  = [np.array([i[1][1] for i in w]) for w in writes.values()]

#     plt.plot(x_axis, [np.mean(i) for i in d_clean])
#     plt.plot(x_axis, [np.mean(i) for i in d_wpts])

#     # print(sizes, [np.mean(i) for i in d_clean])#, d_wpts)
#     # print([w if for w in writes.values()])
#     print()

# plt.xticks(x_axis)
# plt.yscale('log')
# plt.savefig("fig.png")

items = 6

width = (1/(items*2))*.8

labels = ["$1$ variable",
          "$2^4$ variables",
          "$2^8$ variables",
          "$2^{12}$ variables",
          "$2^{16}$ variables",
          "$2^{20}$ variables"]

plt.figure(figsize=(12, 9))

for i, (sizes, writes) in enumerate(data_laptop.items()):

    d_clean = [np.array([v[0][1] for v in w]) for w in writes.values()]

    offset = .9*((1/items) * i - .5 + (width / 2))
    color_offset = .7*(i/(items-1))

    plt.bar(np.arange(4) + offset, [np.mean(v) if v.size != 0 else 0 for v in d_clean], 
            width=width, yerr=[np.std(v) if v.size != 0 else 0 for v in d_clean], 
            capsize=1.5, color=(.94-.2*color_offset, 0, .8-color_offset), 
            label=f"{labels[i]}")

for i, (sizes, writes) in enumerate(data_laptop.items()):

    d_wpts  = [np.array([v[1][1] for v in w]) for w in writes.values()]

    offset = .9*((1/items) * i - .5 + (width / 2))
    color_offset = .7*(i/(items-1))

    plt.bar(np.arange(4) + offset + width, [np.mean(v) if v.size != 0 else 0 for v in d_wpts],
            width=width, yerr=[np.std(v) if v.size != 0 else 0 for v in d_wpts], 
            capsize=1.5, color=(0, .95-.6*color_offset, .4+.2*color_offset),
            label=f"{labels[i]}")

plt.xticks(range(4), ["$2^{16}$", "$2^{18}$", "$2^{20}$", "$2^{22}$"])
plt.yscale('log')
plt.legend(ncol=2, columnspacing=3, title="Without watchpoints      With watchpoints ")
# plt.legend(ncol=2, title="Normal                        Watchpoints")
plt.xlabel("Number of writes", fontsize=12)
plt.ylabel("Time (s)", fontsize=12)
plt.savefig("bar-laptop.png")