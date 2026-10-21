# raycandle

**Native candlestick and time-series charts for Python.** Raycandle uses a C renderer built on [raylib](https://www.raylib.com/) with a Python API backed by CFFI, NumPy, and pandas. It targets local desktop analysis of historical OHLC data and incremental updates from a market-data feed.

This is a visualization library, **not** a trading system or financial-data provider. Example market data is synthetic and reproducible.

## What it does

- Draw candlesticks and overlaid lines in multiple independent views.
- Switch between views/timeframes without copying their plot data.
- Store fixed-capacity plot buffers in C, exposing NumPy/pandas views for analysis.
- Update only changed values, roll a candle forward on incoming ticks, and shift indicator buffers.
- Load and validate timestamped OHLC CSV files for offline visual inspection.

## Try it with your own OHLC data

The CSV viewer expects columns `timestamp,open,high,low,close`, with increasing ISO-8601 timestamps. It validates price ranges and time ordering before plotting.

```sh
python examples/csv_market_viewer.py examples/data/sample_ohlc.csv
```

The included dataset is synthetic. A desktop display and a built native extension are required to open the chart; see **Build** below.

```python
import raycandle as rc
from raycandle.market_data import read_ohlc_csv

bars = read_ohlc_csv("examples/data/sample_ohlc.csv")
fig = rc.Figure(2, 1)
fig.ax[0].plot(rc.Candle(bars))
fig.ax[0].plot(bars.c.rolling(10).mean(), name="SMA 10")
fig.ax[1].plot(bars.c.diff().fillna(0), name="Close change")
try:
    fig.show()  # blocks until the desktop window closes
finally:
    fig.close()
```

For a minimal window use `examples/basic.py`; for multiple independently sampled views use `examples/multi_view.py`. `examples/concurrent_updates.py` is a more involved manual concurrency stress scenario, not an automated test.

## Build

**Prerequisites:** Python 3.9+, a C11 compiler, CMake 3.18+, and a **previously built static raylib** with its headers. Raycandle does **not** download raylib automatically. Install/build raylib separately, then provide its installation path. A display/OpenGL implementation is required when opening the window; the native tests do not need a display.

On Linux, set `RAYLIB_ROOT` to a directory containing `include/raylib.h` and `lib/libraylib.a`, or to a raylib checkout containing `src/raylib.h` and `src/libraylib.a`:

```sh
export RAYLIB_ROOT=/path/to/raylib/src
python -m pip install .
```

On Windows, use an installed MinGW-based raylib build that includes `raylib.h`, `libraylib.a`, and a matching MinGW toolchain (e.g. bundled with raylib). An MSVC `.lib` is not interchangeable with a MinGW `.a`:

```powershell
$env:RAYLIB_ROOT="C:\raylib\raylib\src"
py -m pip install .
```

You can also build and stage the native library manually, without installing the Python package:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRAYLIB_ROOT=/path/to/raylib/src
cmake --build build --config Release
cmake --install build --config Release --prefix .
```

On Windows with multiple CMake generators installed, pass a generator compatible with your raylib archive, e.g. `-G "MinGW Makefiles"` **at configure time** (and ensure its make/compiler tools are on `PATH`). `cmake --build` operates on a *previously configured* directory; `cmake --install` stages the native artifact and CFFI header in `raycandle/`. The Linux root `Makefile` is only a wrapper around these CMake commands.

If the native header/library is missing, `Figure()` raises an actionable error. Python imports, CSV loading, and the headless validation tests do not require a native build.

## Views and live updates

Each `Figure` starts with one empty view. An `Axes` belongs to its parent view; `fig.ax` always refers to the active view. `add_view` selects the new view and optionally initializes its timestamps but does not draw any artists automatically.

```python
fig = rc.Figure(2, 1)
fig.ax[0].plot(rc.Candle(bars))
second = fig.add_view(bars.iloc[::5], title="5-minute sample", timeframe=300)
second.ax[0].plot(rc.Candle(bars.iloc[::5]))
fig.select_view(0)
```

Once plotted, artists own fixed-capacity native buffers. `Candle.update_tick(epoch, price)` updates the latest bar or rolls to a new interval. `Line.shift()` discards old indicator values and `Line.set_data(values, start)` updates a small slice. `Collector.update_tail(df.tail(n))` writes aligned changed rows across artists in one view. Time indexes use float64 Unix seconds (or numeric axis units); price buffers use float32.

**Memory ownership:** `artist.data`, `artist.ydata`, and `view.epochs` reference C-owned memory. They become invalid after `fig.close()`. With a running window, use `.set_data()`/`.update_tick()` for synchronized changes or `with fig.edit():` for direct NumPy/pandas writes. Do not acquire these non-recursive locks within each other. Plot artists and create views **before** calling `show()`.

`show(block=False)` runs the event loop on a worker thread for supported systems; GUI libraries may require the main thread. macOS requires `show(block=True)` and is not a supported build target. Close the window and call `wait_closed()` before releasing a nonblocking figure.

## Tests and project structure

```sh
python -m unittest discover -s tests -v
# or
python -m pytest -q
```

Pure-Python validation tests run without raylib. The native integration suite runs when `raycandle/raycandle.h` and the platform's compiled native library are present; otherwise it is explicitly skipped. In CI these artifacts are built and checked before tests run. See [testing and limitations](docs/testing.md) and [architecture](docs/architecture.md).

The core renderer lives under `csrc/`; Python API modules are in `raycandle/`; example inputs and viewers are in `examples/`; regression tests are in `tests/`. Optional network feed prototypes are in `raycandle/cs/` and may require extra dependencies or external service credentials; no broker API is needed for the included offline example.

## Limitations

- The native window requires a compatible raylib/OpenGL environment; no graphical tests run on headless CI.
- Buffers cannot be resized after artist creation; construct a new view/figure for another history length.
- The included example prices are synthetic; no investment decisions should be based on them.
- Public Python interfaces are evolving; CFFI bindings require a matching binary/header build.
