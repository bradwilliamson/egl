# EGL Unit Tests

This folder contains unit tests for the EGL Quake 2 engine. The goals are:

- Prevent regressions in core subsystems
- Make tricky edge-cases reproducible
- Provide fast feedback in CI

## Quick Start

From the repo root:

```sh
# Build + run tests (CI style, writes per-test logs)
make -C tests ci

# Clean
make -C tests clean
```

On Windows without MSYS2, you can still run already-built test executables:

```bat
run_all_tests.bat
```

## Test Framework

We use a minimal single-header Unity-style test framework at `../unity/unity.h`.

- Simple assertions (`TEST_ASSERT_EQUAL`, `TEST_ASSERT_FLOAT_WITHIN`, etc.)
- Colorized pass/fail output
- Zero external dependencies

## Adding New Tests

1. Create `test_<module>.c` in this folder following the existing pattern.
2. Add a build rule to `tests/Makefile` and add the binary name to `TEST_BINS`.
3. Run `make -C tests ci`.

## CI

Unit tests run in GitHub Actions as part of the SDL2 CI workflow:

- `.github/workflows/ci-sdl2.yml`

Logs are uploaded as artifacts (`tests/*.log`, `tests/test-output.log`).
