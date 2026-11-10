#!/usr/bin/env python3
"""Confidence scorer comparison on the eight VeReMi 0709 fault profiles (paper Table III).

The checker output is the same for every scorer; only the scorer changes (--trust):
  BetaReputation  counterless Beta confidence (proposed): pseudo-counts rebuilt from S with
                  constant evidence mass W_p, severity-weighted negative evidence
  EWMA            exponentially weighted moving average with the same prior weight W_p
  MajorityVote    majority of pass/fail verdicts over a sliding window of W messages
Reports recall, precision, F1, FPR and ROC AUC (anomaly score 1 - S) per profile and their
macro averages, and the eviction latency: for each (receiver, attacker) pair, the number of receptions until the
first S < 0.5 (VeReMi attackers misbehave from their first beacon). Per profile, the median
and 90th percentile (pairs never flagged count as infinite) and the share of pairs never
flagged; per scorer, the mean of the per-profile medians, excluding ConstPosOffset.

Usage: python run_scorer_comparison.py [--workers 2] [--output-dir DIR]
"""
import argparse
import io
import os
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor

import numpy as np
import pandas as pd
from sklearn.metrics import roc_auc_score

import run_full_detection_eval as rca
from results_io import default_dir, start_log

SCORERS = ["BetaReputation", "EWMA", "MajorityVote"]
LATENCY_EXCLUDED = "ConstPosOffset_0709"  # constant offset: almost never flagged


def run_dir(sim_dir, scorer, w):
    """Confusion counts, ROC inputs and per-attacker-pair eviction latencies of one hourly directory."""
    att = rca.extract_attackers(sim_dir)
    out = subprocess.run([rca.RUNNER_BIN, "--dir", sim_dir, "--historySize", str(w), "--trust", scorer],
                         capture_output=True, text=True, check=True).stdout
    d = pd.read_csv(io.StringIO(out), usecols=["receiver", "sender", "score"])
    a = d.sender.isin(att); f = d.score < 0.5
    counts = dict(tp=int((f & a).sum()), fp=int((f & ~a).sum()), tn=int((~f & ~a).sum()), fn=int((~f & a).sum()))
    roc = (a.to_numpy(np.int8), (1.0 - d.score.to_numpy(np.float32)))
    d = d[a].assign(k=d[a].groupby(["receiver", "sender"]).cumcount() + 1, flag=f[a])
    first = d[d.flag].groupby(["receiver", "sender"]).k.min()
    pairs = d.groupby(["receiver", "sender"]).size().index
    lat = first.reindex(pairs).fillna(np.inf).to_numpy()
    return counts, roc, lat


def run(scenario, scorer, w):
    """Pooled metrics and eviction latency of one scorer on one fault profile."""
    t, lats, ys, ss = dict(tp=0, fp=0, tn=0, fn=0), [], [], []
    for d in rca.find_parquet_dirs(scenario):
        c, (y, s), lat = run_dir(d, scorer, w)
        lats.append(lat); ys.append(y); ss.append(s)
        for k in t:
            t[k] += c[k]
    lat = np.concatenate(lats)
    p = t["tp"] / max(1, t["tp"] + t["fp"])
    r = t["tp"] / max(1, t["tp"] + t["fn"])
    return {"scenario": scenario, "scorer": scorer, "recall": 100 * r, "precision": 100 * p,
            "f1": 2 * p * r / (p + r) if p + r else 0.0,
            "fpr": 100 * t["fp"] / max(1, t["fp"] + t["tn"]),
            "roc_auc": float(roc_auc_score(np.concatenate(ys), np.concatenate(ss))),
            "lat_median": float(np.percentile(lat, 50, method="higher")), "lat_p90": float(np.percentile(lat, 90, method="higher")),
            "never_flagged_pct": float(100 * np.isinf(lat).mean()), "pairs": int(lat.size), **t}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scenarios", nargs="+", default=rca.ALL_0709_SCENARIOS)
    ap.add_argument("--history-size", type=int, default=rca.header_history_window())
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--output-dir", default=default_dir("scorers"))
    args = ap.parse_args()
    start_log(args.output_dir, "scorer_comparison")

    tasks = [(sc, s) for sc in args.scenarios for s in SCORERS]
    with ThreadPoolExecutor(args.workers) as pool:
        df = pd.DataFrame(pool.map(lambda t: run(*t, args.history_size), tasks))

    cols = ["recall", "precision", "f1", "fpr", "roc_auc", "lat_median", "lat_p90", "never_flagged_pct"]
    per = df.pivot(index="scenario", columns="scorer", values=cols).reindex(args.scenarios)
    macro = df.groupby("scorer")[cols[:5]].mean().reindex(SCORERS)
    macro["lat_median_macro"] = df[df.scenario != LATENCY_EXCLUDED].groupby("scorer").lat_median.mean()
    print("Per profile:\n" + per.round(4).to_string())
    print("\nMacro averages (recall, precision, FPR in %; latency in receptions, without "
          f"{LATENCY_EXCLUDED}):\n" + macro.round(4).to_string())

    stamp = time.strftime("%Y%m%d_%H%M%S")
    df.to_csv(os.path.join(args.output_dir, f"scorer_comparison_{stamp}.csv"), index=False)
    md = ("# Scorer comparison (VeReMi 0709)\n\n## Per profile\n\n```\n" + per.round(4).to_string()
          + "\n```\n\n## Macro averages\n\n```\n" + macro.round(4).to_string() + "\n```\n")
    for name in (f"scorer_comparison_{stamp}.md", "scorer_comparison_latest.md"):
        with open(os.path.join(args.output_dir, name), "w") as f:
            f.write(md)
    print(f"\nSaved to {args.output_dir}")


if __name__ == "__main__":
    main()
