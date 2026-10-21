# Test strategy and current limits

The repository uses Python's `unittest` runner (also discovered by `pytest`). Tests are divided into two groups:

| Suite | Requires native library | What it validates |
| --- | --- | --- |
| `tests/test_validation.py` | No | Timestamp ordering, epoch precision, timeframe validation, CSV schema and OHLC constraints |
| `tests/test_core.py` | Yes | Multi-view independence, C buffer aliasing, figure creation, tick rollover, line shifting and Collector tail updates |

The native test module is **skipped**, not silently passed, if a platform-specific library or generated CFFI header is unavailable. CI builds both prerequisites and asserts the files exist first. Run locally after building raylib and Raycandle:

```sh
python -m unittest discover -s tests -v
```

## Regression test examples

- Non-monotonic or duplicate timestamps are rejected rather than corrupting the native x-axis.
- Partial timestamp writes are checked against untouched neighbors.
- Malformed OHLC bars (high below close, low above open, non-finite prices) are rejected during CSV ingestion.
- Native integration tests check updates to only the changed tail and whether NumPy views refer to C-allocated memory.

## Not covered automatically

Graphics correctness, mouse/key input, rendering under an actual display server, Windows toolchain compatibility, and true threaded/window stress scenarios need further validation. Run `examples/multi_view.py` or `examples/concurrent_updates.py` manually on the target OS; they are examples, not CI evidence.

When reporting a bug, include the OS/compiler, Python/CMake/raylib versions, minimal input data, expected vs actual behavior, and whether the issue reproduces without opening a window.
