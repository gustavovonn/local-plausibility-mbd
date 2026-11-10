# Computational Cost Evaluation

`offline_evaluator` measures the per-message latency of the proposed mechanism and of the F2MD
check suites and decision applications (re-executed offline without simulator context) on
VeReMi Extension traces, in three tiers: Checker, Scorer, and Checker + Scorer (paper Table IV).
The mechanism and its constants come from the repository's `include/` and `src/`.

## 1. Prepare the traces (host)

Requires the Parquet datalake from `veremi-extension/run_pipeline.sh`.

```bash
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
.venv/bin/python3 scripts/interpolate_veremi.py      # raw JSON -> synchronized Parquet (veremi/parquet/)
.venv/bin/python3 scripts/export_parquet_to_bin.py   # Parquet -> zero-dependency .bin (veremi/bin/)
```

## 2. Build and deploy to the VisionFive 2 (host)

```bash
JOBS=2 ../vf2-setup/vf2_setup.sh --rebuild-project
```

Cross-compiles a static RISC-V `offline_evaluator`, packs it with the `.bin` traces and the
benchmark runner into an initramfs, and serves kernel, device tree and initramfs over TFTP.

## 3. Run on the board

```bash
cd /bench && ./run_benchmarks_riscv.sh --max-msgs 50000
```

Runs the three tiers for every configuration of Table IV on the eight 0709 profiles and writes
`results/benchmark_riscv_<timestamp>.csv` (mean latencies). `--profile ConstPos_0709` runs a
single profile. The same script also runs on the host from `vf2-setup/`.
