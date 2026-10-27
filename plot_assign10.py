# Usage: python3 plot_assign10.py   (needs matplotlib; reads routing_results.csv)
import csv
import matplotlib.pyplot as plt
rows = list(csv.DictReader(open("routing_results.csv")))
conds = ["Normal", "Congestion", "LinkFailure"]
protos = ["RIP", "OSPF", "AODV"]
for col, title, fn in [("pdr", "PDR (%)", "pdr.png"),
                       ("delay_ms", "End-to-End Delay (ms)", "delay.png"),
                       ("updates", "Routing Table Updates", "updates.png")]:
    plt.figure()
    w = 0.25
    for i, p in enumerate(protos):
        vals = [float(r[col]) for c in conds for r in rows if r["protocol"] == p and r["condition"] == c]
        plt.bar([x + i * w for x in range(3)], vals, w, label=p)
    plt.xticks([x + w for x in range(3)], conds)
    plt.xlabel("Network Condition"); plt.ylabel(title); plt.title(title + " vs Network Condition")
    plt.legend(); plt.savefig(fn, dpi=150)
print("Saved pdr.png delay.png updates.png")
