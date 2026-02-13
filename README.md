# Benchmark HiGHS

Small C++ benchmark driver for solving LP problems with [HiGHS](https://github.com/ERGO-Code/HiGHS).

It reads problems in CSC format, builds a `HighsLp` model, runs HiGHS (simplex, no presolve, 1 thread), and prints objective values.

## Requirements

- Linux/macOS environment
- CMake >= 3.15
- C++17 compiler
- A local HiGHS installation discoverable by CMake

## 1) Install HiGHS

This repository expects HiGHS under `$HOME/highs-install` by default.

```bash
git clone https://github.com/ERGO-Code/HiGHS.git
cd HiGHS
rm -rf build
mkdir build && cd build

cmake .. \
  -DCMAKE_INSTALL_PREFIX=$HOME/highs-install \
  -DCMAKE_BUILD_TYPE=Release

make -j
make install
```

## 2) Build this project

From the repository root:

```bash
rm -rf build
cmake -S . -B build -Dhighs_DIR=$HOME/highs-install/lib/cmake/highs
cmake --build build -j
```

This creates the executable:

- `build/solve`

## 3) Run

### Run on one file

```bash
./build/solve /path/to/problem.csc
```

### Run on a directory

```bash
./build/solve /path/to/problem_directory
```

The program processes files ending in `.csc` or `.txt`.

## Input format

The reader expects this structure:

1. Matrix header: `<name> csc <m> <n> <nnz>`
2. `col_ptr` with `n+1` integers
3. `row_idx` with `nnz` integers
4. `values` with `nnz` numbers
5. RHS header: `<name> dense <m>` then `m` numbers
6. Objective header: `<name> dense <n>` then `n` numbers

## Batch script

`testing.sh` rebuilds and launches batch runs with `srun` (HPC/Slurm workflow).

Before running it, review and adjust:

- `DATA_DIR`
- `ACCOUNT`
- `SRUN_TIME`
- `TIMEOUT`

Then run:

```bash
bash testing.sh
```

Outputs are written under the configured results directory (for example `results/`).

## Notes

- Solver options are set in `testing.cpp`.
- Current defaults: `simplex`, `presolve=off`, `threads=1`, console logging disabled.
- If CMake cannot find HiGHS, pass `-Dhighs_DIR=...` explicitly as shown above.
