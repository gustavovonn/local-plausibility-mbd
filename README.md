# A Lightweight Mechanism to Attribute Confidence to V2X Messages based on Physical Plausibility

Receiver-side confidence attribution for V2X kinematic beacons, evaluated offline on the
[VeReMi Extension](https://zenodo.org/records/20090854) dataset: detection quality on the host and
computational cost on a StarFive VisionFive 2 (RISC-V), both outside of OMNeT++.

## Repository Layout

- `include/`, `src/` — the mechanism (Predictor, Checker, Scorer) and its constants
  (`include/MechanismConstants.h`), shared by both experiments.
- `libs/v2x-misc/` — header-only motion primitives (coordinate policies, CTRA/CA integration,
  OLS kinematic regression).
- `veremi-extension/` — dataset pipeline: download, extraction and conversion to Parquet.
- `detection-eval/` — detection experiment (`veremi_runner` and the paper's analysis scripts).
- `computational-eval/` — cost experiment (`offline_evaluator`, with a decoupled F2MD reference).
- `vf2-setup/` — cross-compilation and netboot packaging for the VisionFive 2.
- `reproduce_paper.sh` — runs every detection result of the paper.

## Prerequisites

CMake 3.16+, a C++20 compiler and Python 3. Apache Arrow and Parquet for C++ (`libarrow-dev`,
`libparquet-dev`) for `detection-eval`. For `vf2-setup`: a RISC-V cross toolchain
(`riscv64-linux-gnu-gcc`/`g++`), `cpio`, `gzip` and a TFTP server. Tested on Ubuntu 24.04.4 LTS.
The scripts below create each module's Python virtual environment on first run.

## Reproducing the Paper

```bash
# 1. Dataset: download, extract and convert to Parquet
./veremi-extension/run_pipeline.sh

# 2. Detection results (Tables I-III): window selection, then copy W* and its weights into
#    include/MechanismConstants.h, then every other result (detection-eval/results/paper/)
./reproduce_paper.sh select
./reproduce_paper.sh results

# 3. Computational cost (Table IV) on the VisionFive 2
JOBS=2 ./vf2-setup/vf2_setup.sh --rebuild-project   # then on the board: cd /bench && ./run_benchmarks_riscv.sh
```

Details: [detection-eval/README.md](detection-eval/README.md) and
[computational-eval/README.md](computational-eval/README.md).

## References

- [F2MD (Framework For Misbehavior Detection)](https://github.com/josephkamel/F2MD)
- [VeReMi Extension Dataset](https://zenodo.org/records/20090854)
- [OMNeT++](https://omnetpp.org/)
