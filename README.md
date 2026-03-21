# GCC Trunk – C++ Special Members Lab

A portable Docker lab for exploring the **Rule of Zero, Three, and Five** in
modern C++ using **GCC trunk** (nightly snapshot).

## Quick start

```bash
# Build the image and drop into an interactive shell
./run.sh

# Inside the container – run everything at once
./run_all.sh

# Or compile individually with make
make && ./bin/01_rule_of_zero
```

## Commands

| Command | Description |
|---------|-------------|
| `./run.sh` | Build image if needed, then start interactive shell |
| `./run.sh build` | (Re)build the Docker image |
| `./run.sh run` | Start interactive shell |
| `./run.sh demo` | Build + run all examples non-interactively |
| `./run.sh clean` | Remove container and image |
| `./run.sh push` | Push to Docker Hub (needs `DOCKER_USER` env var) |

## Examples

| File | Topic |
|------|-------|
| `01_rule_of_zero.cpp` | RAII types; let compiler generate everything |
| `02_rule_of_three.cpp` | Custom destructor + copy ctor + copy assign |
| `03_rule_of_five.cpp` | Adding move ctor + move assign; `noexcept` |
| `04_move_performance.cpp` | Measurable speedup of move vs copy |
| `05_raii_wrappers.cpp` | RAII wrappers for `FILE*` and raw arrays |
| `06_common_pitfalls.cpp` | Double-free, missing `noexcept`, unsafe assign |

## How it works

The `Dockerfile` installs the `gcc-snapshot` package from the
[Ubuntu Toolchain PPA](https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test),
which provides a pre-compiled nightly build of GCC trunk.

All examples compile with:
```
g++ -std=c++23 -Wall -Wextra -O2
```

The `examples/` directory is **live-mounted** into the container, so you can
edit files on the host and recompile inside the container without rebuilding
the image.

## Push to GitHub

```bash
git init
git add .
git commit -m "init: GCC trunk C++ special members lab"
gh repo create gcc-cpp-rules-lab --public --source=. --remote=origin --push
```

GitHub Actions (`.github/workflows/docker-build.yml`) will automatically
build the image and run all examples on every push to `main`.
