import os
import re
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from collections import Counter
from matplotlib.ticker import FuncFormatter

import data

MiB = 1024 * 1024

def to_mib_s(x):
    return np.asarray(x, dtype=float) / MiB

def short(s, n=40):
    s = str(s)
    return s if len(s) <= n else s[:n-1] + "…"

def is_unknown_client(name: str) -> bool:
    return name.startswith("Unknown")

def slugify(s: str, maxlen=120) -> str:
    s = str(s)
    s = re.sub(r"\s+", "_", s.strip())
    s = re.sub(r"[^A-Za-z0-9._-]+", "", s)
    return s[:maxlen] if len(s) > maxlen else s

def build_df(rows):
    df = pd.DataFrame(rows).copy()
    df["avg_dl_mib"] = df["avg_dl"] / MiB
    df["peak_dl_mib"] = df["peak_dl"] / MiB
    df["avg_ul_mib"] = df["avg_ul"] / MiB
    df["peak_ul_mib"] = df["peak_ul"] / MiB
    df["short_name"] = df["name"].map(lambda x: short(x))
    df["avg_leechers"] = (df["avg_peers"].astype(float) - df["avg_seeds"].astype(float)).clip(lower=0)
    return df

def normalize_client(raw: str) -> str:
    if raw is None:
        return "Unknown"
    s = str(raw).strip()
    if not s:
        return "Unknown"

    s = s.replace("μ", "u").replace("µ", "u")
    s = s.replace("/", " ")
    s = re.sub(r"\s+", " ", s).strip()
    s = s.split()[0]

    return s

def normalize_client_counts(client_counts: dict) -> dict:
    c = Counter()
    for k, v in (client_counts or {}).items():
        c[normalize_client(k)] += int(v)
    return dict(c)

def compute_client_matrix(rows, top_k, normalize="row"):
    trackers = [r["tracker"] for r in rows]

    total_counts = {}
    for r in rows:
        cc = r.get("client_counts", {}) or {}
        for k, v in cc.items():
            if is_unknown_client(k):
                continue
            total_counts[k] = total_counts.get(k, 0) + int(v)

    clients = [k for k, _ in sorted(total_counts.items(), key=lambda kv: kv[1], reverse=True)[:top_k]]

    M = np.zeros((len(rows), len(clients)), dtype=float)
    unknown_share = np.zeros(len(rows), dtype=float)

    for i, r in enumerate(rows):
        cc = r.get("client_counts", {}) or {}
        total = float(sum(cc.values())) if cc else 0.0
        unk = sum(v for k, v in cc.items() if is_unknown_client(k))
        unknown_share[i] = (unk / total) if total > 0 else 0.0

        for j, c in enumerate(clients):
            M[i, j] = float(cc.get(c, 0))

    if normalize == "row":
        row_sums = M.sum(axis=1, keepdims=True)
        with np.errstate(divide="ignore", invalid="ignore"):
            M = np.where(row_sums > 0, M / row_sums, 0.0)

    return trackers, clients, M, unknown_share

def plot_A_popularity(df, metric):
    df = df.sort_values(metric, ascending=True)

    fig, ax = plt.subplots(figsize=(10, 5))
    ax.barh(df["tracker"], df[metric], color="blue")
    ax.set_title(f"Popularity by {metric}")
    ax.set_xlabel(metric)
    for y, (val, nm) in enumerate(zip(df[metric].values, df["short_name"].values)):
        ax.text(val, y, f"  {nm}", va="center", fontsize=8)
    ax.grid(axis="x", alpha=0.25)
    fig.tight_layout()
    return fig

def plot_B_speed(rows, kind="download", use_log=False):
    jitter = 0.1
    recs = []
    all_samples = []

    if kind == "download":
        title = "Download speed"
    else:
        title = "Upload speed"

    for r in rows:
        tracker = r["tracker"]

        if kind == "download":
            samples = np.asarray(r.get("dl_samples", []), dtype=float) / MiB
        else:
            samples = np.asarray(r.get("ul_samples", []), dtype=float) / MiB

        all_samples.append(samples)

        mean = float(np.mean(samples)) if samples.size else np.nan
        median = float(np.median(samples)) if samples.size else np.nan
        vmax = float(np.max(samples)) if samples.size else np.nan

        recs.append((tracker, samples, mean, median, vmax))

    recs.sort(key=lambda x: x[2])

    fig, ax = plt.subplots(figsize=(10, 5))

    for i, (tracker, samples, mean, median, vmax) in enumerate(recs):
        label = lambda l: l if i == 0 else None

        y = np.full(samples.shape, i, dtype=float)
        y += np.random.uniform(-jitter, jitter, size=samples.size)

        ax.scatter(samples, y, s=12, alpha=0.25, color="black", edgecolors="none", label=label("samples"))

        ax.scatter([mean], [i], s=55, marker="|", color="red", label=label("= mean"))
        ax.scatter([median], [i], s=55, marker="|", color="green", label=label("= median"))
        ax.scatter([vmax], [i], s=65, marker="|", color="orange", linewidths=2.0, label=label("= max"))

    ax.set_yticks(np.arange(len(recs)))
    ax.set_yticklabels([t for t, *_ in recs])
    ax.set_xlabel("MiB/s")
    ax.set_title(title)
    ax.grid(axis="x", alpha=0.25)

    if use_log:
        ax.set_xscale("log")

    ax.legend(loc="lower right")
    fig.tight_layout()
    return fig

def plot_C_seeds_leechers_scatter(df):
    x = df["avg_seeds"].values.astype(float)
    y = df["avg_leechers"].values.astype(float)
    size = df["avg_dl_mib"].values.astype(float)

    spread = np.ptp(size)
    s = 50 + 250 * (size - size.min()) / (spread + 1e-9)

    fig, ax = plt.subplots(figsize=(8, 6))
    sc = ax.scatter(x, y, s=s, c=size, cmap="viridis", alpha=0.85,
                    edgecolor="k", linewidth=0.5)

    ax.set_title("avg_seeds vs avg_leechers (size=avg download MiB/s)")
    ax.set_xlabel("avg_seeds")
    ax.set_ylabel("avg_leechers")
    ax.grid(alpha=0.25)

    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)

    for _, r in df.iterrows():
        ax.text(r["avg_seeds"] + 0.2, r["avg_leechers"] + 0.2, r["tracker"], fontsize=9)

    cbar = fig.colorbar(sc, ax=ax)
    cbar.set_label("avg download (MiB/s)")
    fig.tight_layout()
    return fig

def plot_F_clients_heatmap(rows, top_k):
    trackers, clients, M, unknown_share = compute_client_matrix(rows, top_k)

    order = np.argsort(unknown_share)
    trackers_o = [trackers[i] for i in order]
    M_o = M[order, :]
    unknown_o = unknown_share[order]

    fig = plt.figure(figsize=(12, 6))
    gs = fig.add_gridspec(1, 2, width_ratios=[4, 1], wspace=0.25)

    ax = fig.add_subplot(gs[0, 0])
    vmax = max(0.2, float(M_o.max()))
    im = ax.imshow(M_o, aspect="auto", interpolation="nearest", cmap="Blues", vmin=0.0, vmax=vmax)
    ax.set_title(f"Client mix (top {top_k} known clients)")
    ax.set_yticks(np.arange(len(trackers_o)))
    ax.set_yticklabels(trackers_o)
    ax.set_xticks(np.arange(len(clients)))
    ax.set_xticklabels([short(c, 18) for c in clients], rotation=45, ha="right")
    ax.set_xlabel("Client")
    ax.set_ylabel("Tracker")
    cbar = fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
    cbar.set_label("Share among top known clients")

    ax2 = fig.add_subplot(gs[0, 1])
    ax2.barh(np.arange(len(trackers_o)), unknown_o)
    ax2.set_yticks(np.arange(len(trackers_o)))
    ax2.set_yticklabels([])
    ax2.set_xlim(0, 1)
    ax2.set_title("Unknown")
    ax2.xaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{int(v*100)}%"))
    ax2.grid(axis="x", alpha=0.25)

    fig.tight_layout()
    return fig

def plot_D_per_tracker(rows):
    figs = []
    for r in rows:
        tracker = r["tracker"]
        name = r.get("name", "")

        dl = to_mib_s(r.get("dl_samples", []))
        ul = to_mib_s(r.get("ul_samples", []))
        peers = np.asarray(r.get("peer_counts", []), dtype=float)
        seeds = np.asarray(r.get("seed_counts", []), dtype=float)

        t = np.arange(1, len(dl) + 1)

        fig, (ax1, ax2) = plt.subplots(
            2, 1, figsize=(11, 6), sharex=True,
            gridspec_kw={"height_ratios": [2, 1]}
        )
        fig.suptitle(f"{tracker} - {short(name, 90)}", y=0.98)

        ax1.plot(t, dl, label="DL (MiB/s)", color="#59A14F", linewidth=2)
        ax1.plot(t, ul, label="UL (MiB/s)", color="#E15759", linewidth=1.5, alpha=0.9)
        ax1.set_ylabel("Speed (MiB/s)")
        ax1.grid(alpha=0.25)
        ax1.legend(loc="upper right")

        ax2.plot(t, peers, label="peers", color="#4C78A8", linewidth=2)
        if len(seeds) == len(peers) and len(seeds) > 0:
            ax2.plot(t, seeds, label="seeds", color="#F28E2B", linewidth=2)
        ax2.set_ylabel("Count")
        ax2.set_xlabel("Seconds observed")
        ax2.grid(alpha=0.25)
        ax2.legend(loc="upper right")

        fig.tight_layout()
        figs.append((tracker, fig))
    return figs

def save_all_plots(rows):
    outdir = "./pics"
    top_k_clients = 8
    use_log_speed = False
    dpi = 160

    for row in rows:
        row["client_counts"] = normalize_client_counts(row["client_counts"])

    os.makedirs(outdir, exist_ok=True)
    df = build_df(rows)

    for metric in ["peak_peers", "peak_dl_mib"]:
        fig = plot_A_popularity(df, metric)
        fig.savefig(os.path.join(outdir, f"A_popularity_{metric}.png"), dpi=dpi, bbox_inches="tight")
        plt.close(fig)

    fig = plot_B_speed(rows, kind="download", use_log=use_log_speed)
    fig.savefig(os.path.join(outdir, "B_speed_download.png"), dpi=dpi, bbox_inches="tight")
    plt.close(fig)

    fig = plot_B_speed(rows, kind="upload", use_log=use_log_speed)
    fig.savefig(os.path.join(outdir, "B_speed_upload.png"), dpi=dpi, bbox_inches="tight")
    plt.close(fig)

    fig = plot_C_seeds_leechers_scatter(df)
    fig.savefig(os.path.join(outdir, "C_seeds_vs_leechers.png"), dpi=dpi, bbox_inches="tight")
    plt.close(fig)

    for tracker, fig in plot_D_per_tracker(rows):
        fname = f"D_{slugify(tracker)}_timeseries.png"
        fig.savefig(os.path.join(outdir, fname), dpi=dpi, bbox_inches="tight")
        plt.close(fig)

    fig = plot_F_clients_heatmap(rows, top_k=top_k_clients)
    fig.savefig(os.path.join(outdir, f"F_clients_heatmap_top{top_k_clients}.png"), dpi=dpi, bbox_inches="tight")
    plt.close(fig)

    return outdir

def main():
    save_all_plots(data.data)

if __name__ == "__main__":
    main()
