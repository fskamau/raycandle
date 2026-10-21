# Architecture

Raycandle is a desktop time-series plotting library with two layers:

```text
CSV / pandas / NumPy
        |
  raycandle/market_data.py     input validation (optional)
        |
  Figure -> View -> Axes -> Line/Candle
        |                   |
  timestamp validation    C-owned float32 buffers
        |                   |
        +------ CFFI ABI ----+
                    |
                csrc/*.c
                    |
                 raylib
```

`Figure` owns the native figure and its lifecycle. Each `View` owns timestamps, axes, and a selected-view state; `Axes.plot` creates a native artist tied to one view. `Candle` and `Line` expose NumPy/pandas views of buffers allocated by the C layer. `Collector` coordinates partial buffer updates across artists that share a view. The ABI declarations in `csrc/cffi.h` are staged alongside the compiled native library by CMake.

## Data invariants

- Each populated view has at least two strictly increasing, finite timestamps.
- Epoch timestamps are float64 (not float32); candle and line values occupy float32 buffers.
- Plotted buffers have fixed capacity. Append-like updates are implemented by shifting data inside C and updating the tail.
- An update to only part of an epoch array must remain strictly increasing across its left and right boundaries.
- All artists supplied to a `Collector` must belong to the same view and have compatible capacities. The presence of required columns is checked before any artist is modified.

## Concurrency and ownership

Native code manages synchronization around renderer state and the asynchronous data-update APIs. A Python caller can use `Figure.edit()` for synchronized direct access to shared arrays, but must not call methods that reacquire the same nonrecursive lock inside it. `show()` typically runs the desktop event loop on the caller's thread; `show(block=False)` is subject to OS display/thread restrictions.

Never retain pointers, NumPy arrays, or pandas views backed by C-owned memory after `Figure.close()`. This code does not promise general-purpose cross-process shared memory or GPU-accelerated data transformations.

## Extension boundaries

New input adapters should normalize feed messages to a timestamp plus price/bar values before calling the plotting API. They should not move parsing, authentication, or network I/O into the rendering C layer. The `raycandle/cs` modules are optional provider-specific experiments, not needed for the offline viewer.
