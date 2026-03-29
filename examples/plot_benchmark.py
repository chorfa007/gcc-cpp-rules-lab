#!/usr/bin/env python3
"""
plot_benchmark.py — 5-way JSON benchmark: Reflection vs nlohmann / RapidJSON / Boost.JSON / Manual
Usage:
    python3 plot_benchmark.py              # runs Docker, parses live output
    python3 plot_benchmark.py --no-docker  # uses hardcoded fallback values
"""

import sys
import re
import subprocess
import argparse
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np
from pathlib import Path

# ─── colour palette ───────────────────────────────────────────────────────────
COLORS = {
    "Reflection":  "#4C72B0",   # blue
    "Manual":      "#DD8452",   # orange
    "nlohmann":    "#55A868",   # green
    "RapidJSON":   "#C44E52",   # red
    "Boost.JSON":  "#8172B2",   # purple
}
C_GRID = "#E0E0E0"
C_BG   = "#F8F8F8"
C_TEXT = "#2C2C2C"

LABELS = ["Reflection\n(P2996)", "Manual\n(hand-written)", "nlohmann\n/json", "RapidJSON", "Boost.JSON"]
KEYS   = ["Reflection", "Manual", "nlohmann", "RapidJSON", "Boost.JSON"]

# ─── hardcoded fallback (last Docker run) ─────────────────────────────────────
FALLBACK = [
    {"key": "Reflection", "ms": 358.89, "ops": 41.80},
    {"key": "Manual",     "ms": 252.99, "ops": 59.29},
    {"key": "nlohmann",   "ms": 438.51, "ops": 34.21},
    {"key": "RapidJSON",  "ms": 100.47, "ops": 149.30},
    {"key": "Boost.JSON", "ms": 141.23, "ops": 106.21},
]

# ─── run & parse ──────────────────────────────────────────────────────────────

def run_benchmark() -> str:
    cmd = [
        "docker", "run", "--rm",
        "-v", f"{Path(__file__).parent}:/workspace/examples",
        "cpp-reflection-lab:gcc",
        "bash", "-c",
        "cd /workspace/examples && ./bin/20_json_reflection",
    ]
    print("[ Running benchmark in Docker … ]")
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=180)
    if r.returncode != 0:
        raise RuntimeError(r.stderr)
    return r.stdout


def parse_output(text: str) -> list[dict]:
    """Extract the embedded CSV block."""
    m = re.search(r"--- BENCHMARK_CSV ---\nmethod,ms,ops_per_sec\n(.*?)--- END_CSV ---",
                  text, re.DOTALL)
    if not m:
        raise ValueError("CSV block not found in output")

    key_map = {
        "1. Reflection": "Reflection",
        "2. Manual":     "Manual",
        "3. nlohmann":   "nlohmann",
        "4. RapidJSON":  "RapidJSON",
        "5. Boost.JSON": "Boost.JSON",
    }
    results = []
    for line in m.group(1).strip().splitlines():
        parts = line.split(",")
        raw_label = parts[0]
        ms  = float(parts[1])
        ops = float(parts[2])
        key = next((v for k, v in key_map.items() if raw_label.startswith(k)), raw_label)
        results.append({"key": key, "ms": ms, "ops": ops})
    return results

# ─── plotting ─────────────────────────────────────────────────────────────────

def make_figure(data: list[dict], out: Path):
    ms_vals  = [d["ms"]  for d in data]
    ops_vals = [d["ops"] for d in data]
    keys     = [d["key"] for d in data]
    colors   = [COLORS[k] for k in keys]
    x        = np.arange(len(data))
    reflect_ms = ms_vals[0]

    fig = plt.figure(figsize=(18, 11), facecolor=C_BG)
    fig.suptitle(
        "C++26 P2996 Reflection  vs  nlohmann/json · RapidJSON · Boost.JSON · Manual\n"
        "Full pipeline: raw JSON string → Dataset struct  ·  15 passes × 10 000 records  ·  GCC trunk  -O2",
        fontsize=13, fontweight="bold", color=C_TEXT, y=0.98,
    )

    gs = fig.add_gridspec(2, 3, hspace=0.48, wspace=0.38,
                          left=0.06, right=0.97, top=0.88, bottom=0.10)

    ax_ops    = fig.add_subplot(gs[0, :2])   # wide: throughput
    ax_ms     = fig.add_subplot(gs[0, 2])    # latency
    ax_ratio  = fig.add_subplot(gs[1, 0])    # overhead ratio vs Reflection
    ax_rank   = fig.add_subplot(gs[1, 1])    # horizontal rank bar
    ax_pct    = fig.add_subplot(gs[1, 2])    # % faster/slower pie-ish

    def style(ax, title, ylabel=""):
        ax.set_facecolor(C_BG)
        ax.set_title(title, fontsize=10, fontweight="semibold", color=C_TEXT, pad=8)
        ax.tick_params(colors=C_TEXT, labelsize=8)
        ax.spines[["top","right"]].set_visible(False)
        ax.spines[["left","bottom"]].set_color(C_GRID)
        ax.yaxis.grid(True, color=C_GRID, linewidth=0.7, zorder=0)
        ax.set_axisbelow(True)
        if ylabel:
            ax.set_ylabel(ylabel, fontsize=8, color=C_TEXT)

    def blabel(ax, bars, fmt="{:.0f}", offset_pct=0.02):
        for b in bars:
            h = b.get_height()
            ax.text(b.get_x() + b.get_width()/2, h + max(h*offset_pct, 0.3),
                    fmt.format(h), ha="center", va="bottom",
                    fontsize=7.5, color=C_TEXT, fontweight="bold")

    w = 0.55

    # ── Panel 1 (wide): Throughput ops/s ─────────────────────────────────────
    bars = ax_ops.bar(x, ops_vals, width=w, color=colors, zorder=3, edgecolor="white", linewidth=0.8)
    style(ax_ops, "Throughput  (ops / second — higher is faster)", "ops / second")
    ax_ops.set_xticks(x)
    ax_ops.set_xticklabels(LABELS, fontsize=9)
    ax_ops.set_ylim(0, max(ops_vals) * 1.28)
    blabel(ax_ops, bars, "{:.0f}")

    # mark reflection as baseline
    ax_ops.axhline(ops_vals[0], color=COLORS["Reflection"], linestyle="--",
                   linewidth=1.2, alpha=0.6, label=f"Reflection baseline ({ops_vals[0]:.0f} ops/s)")
    ax_ops.legend(fontsize=8, framealpha=0.8, loc="upper right")

    # ── Panel 2: Latency ms ──────────────────────────────────────────────────
    bars = ax_ms.bar(x, ms_vals, width=w, color=colors, zorder=3, edgecolor="white", linewidth=0.8)
    style(ax_ms, "Latency  (ms — lower is faster)", "milliseconds (15 passes)")
    ax_ms.set_xticks(x)
    ax_ms.set_xticklabels(LABELS, fontsize=7)
    ax_ms.set_ylim(0, max(ms_vals) * 1.28)
    blabel(ax_ms, bars, "{:.0f} ms")

    # ── Panel 3: Ratio vs Reflection ─────────────────────────────────────────
    ratios = [d["ms"] / reflect_ms for d in data]
    ratio_colors = [COLORS["Reflection"] if r <= 1.05
                    else "#27AE60" if r < 1.0
                    else "#E07B54"
                    for r in ratios]
    # override: faster than reflection → green
    ratio_colors = [("#27AE60" if r < 0.99 else COLORS["Reflection"] if r <= 1.05 else "#E07B54")
                    for r in ratios]

    bars = ax_ratio.bar(x, ratios, width=w, color=ratio_colors, zorder=3,
                        edgecolor="white", linewidth=0.8)
    ax_ratio.axhline(1.0, color=C_TEXT, linewidth=1.5, linestyle="--",
                     label="1.0x = same as Reflection")
    style(ax_ratio, "Overhead vs Reflection\n(ratio of ms — 1.0x = equal)", "× factor")
    ax_ratio.set_xticks(x)
    ax_ratio.set_xticklabels(LABELS, fontsize=7)
    ax_ratio.set_ylim(0, max(ratios) * 1.3)
    ax_ratio.legend(fontsize=7, framealpha=0.8)
    for b, r in zip(bars, ratios):
        ax_ratio.text(b.get_x() + b.get_width()/2, r + 0.02,
                      f"{r:.2f}×", ha="center", va="bottom",
                      fontsize=8, fontweight="bold", color=C_TEXT)

    # ── Panel 4: Horizontal rank chart (sorted by speed) ─────────────────────
    order = sorted(range(len(data)), key=lambda i: ops_vals[i], reverse=True)
    ranked_labels = [LABELS[i] for i in order]
    ranked_ops    = [ops_vals[i] for i in order]
    ranked_colors = [colors[i]  for i in order]

    bars_h = ax_rank.barh(np.arange(len(data)), ranked_ops,
                          color=ranked_colors, zorder=3, edgecolor="white", linewidth=0.8)
    ax_rank.set_facecolor(C_BG)
    ax_rank.set_title("Speed Ranking  (ops/s, fastest → slowest)",
                      fontsize=10, fontweight="semibold", color=C_TEXT, pad=8)
    ax_rank.tick_params(colors=C_TEXT, labelsize=8)
    ax_rank.spines[["top","right"]].set_visible(False)
    ax_rank.spines[["left","bottom"]].set_color(C_GRID)
    ax_rank.xaxis.grid(True, color=C_GRID, linewidth=0.7, zorder=0)
    ax_rank.set_axisbelow(True)
    ax_rank.set_yticks(np.arange(len(data)))
    ax_rank.set_yticklabels([f"#{i+1}  {ranked_labels[i]}" for i in range(len(data))],
                             fontsize=8)
    ax_rank.invert_yaxis()
    ax_rank.set_xlabel("ops / second", fontsize=8, color=C_TEXT)
    for b, v in zip(bars_h, ranked_ops):
        ax_rank.text(v + max(ranked_ops)*0.01, b.get_y() + b.get_height()/2,
                     f"{v:.0f}", va="center", fontsize=8, color=C_TEXT, fontweight="bold")

    # ── Panel 5: % relative to fastest ───────────────────────────────────────
    fastest = max(ops_vals)
    pct     = [v / fastest * 100 for v in ops_vals]

    bars = ax_pct.bar(x, pct, width=w, color=colors, zorder=3, edgecolor="white", linewidth=0.8)
    style(ax_pct, "Efficiency  (% of fastest library)", "% of fastest")
    ax_pct.set_xticks(x)
    ax_pct.set_xticklabels(LABELS, fontsize=7)
    ax_pct.set_ylim(0, 125)
    ax_pct.axhline(100, color=C_TEXT, linewidth=1.2, linestyle="--", alpha=0.5)
    blabel(ax_pct, bars, "{:.0f}%")

    # ── legend ────────────────────────────────────────────────────────────────
    legend_patches = [mpatches.Patch(facecolor=COLORS[k], label=lbl.replace("\n", " "))
                      for k, lbl in zip(KEYS, LABELS)]
    fig.legend(handles=legend_patches, loc="lower center", ncol=5,
               fontsize=9, framealpha=0.85, bbox_to_anchor=(0.5, 0.01))

    fig.savefig(out, dpi=150, bbox_inches="tight", facecolor=C_BG)
    print(f"[ Saved → {out} ]")

# ─── main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-docker", action="store_true")
    args = parser.parse_args()

    if args.no_docker:
        print("[ Using hardcoded fallback values ]")
        data = FALLBACK
    else:
        try:
            raw = run_benchmark()
            print(raw)
            data = parse_output(raw)
        except Exception as e:
            print(f"[ Docker failed: {e} — using fallback ]", file=sys.stderr)
            data = FALLBACK

    out = Path(__file__).parent / "benchmark_chart.png"
    make_figure(data, out)
    print(f"\nOpen with:  open {out}")

if __name__ == "__main__":
    main()
