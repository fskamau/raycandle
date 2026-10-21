"""Continuous concurrent live test for raycandle.

Two populated real-time views (plus an unused default view).
Each mock API emits five ticks per candle, then advances its own timestamp
by its timeframe. Close the figure to stop both producers.
"""
import threading
import time
import traceback

import numpy as np
import pandas as pd
import raycandle as rc

HISTORY = 160
TICKS_PER_CANDLE = 5
BATCH_DELAY = 0.20  # real seconds between mock API batches; lower to stress CPU
START = pd.Timestamp('2026-01-05 09:00:00', tz='UTC')
stop = threading.Event()
errors = []


def history(freq, seed, initial):
    rng = np.random.default_rng(seed)
    index = pd.date_range(end=START, periods=HISTORY, freq=freq)
    close = initial * np.exp(np.cumsum(rng.normal(0, 0.002, HISTORY)))
    op = np.r_[close[0], close[:-1]]
    spread = rng.uniform(0.0005, 0.003, HISTORY)
    return pd.DataFrame({
        'o': op,
        'h': np.maximum(op, close) * (1 + spread),
        'l': np.minimum(op, close) * (1 - spread),
        'c': close,
    }, index=index)


def sma20(close):
    return float(np.mean(close[-20:]))


def macd_values(close):
    series = pd.Series(close, dtype='float64')
    fast = series.ewm(span=12, adjust=False).mean()
    slow = series.ewm(span=26, adjust=False).mean()
    macd = fast - slow
    signal = macd.ewm(span=9, adjust=False).mean()
    return float(macd.iloc[-1]), float(signal.iloc[-1])


h1 = history('1min', 101, 100.0)
h10 = history('10min', 202, 250.0)

fig = rc.Figure(3, 2)

# Explicitly create BOTH timeframed views. Figure() already owns its default view;
# leaving it empty avoids accidentally using its default timeframe (often 1 second).
v1 = fig.add_view(h1, timeframe=60, title='LIVE 1m')
a1 = fig.ax
c1 = a1[0].plot(rc.Candle(h1[['o', 'h', 'l', 'c']]))
sma_series = h1.c.rolling(20).mean()
sma = a1[0].plot(sma_series, name='SMA20', lw=2)
a1[1].plot(h1.c, name='Initial close')

v10 = fig.add_view(h10, timeframe=600, title='LIVE 10m')
a10 = fig.ax
c10 = a10[0].plot(rc.Candle(h10[['o', 'h', 'l', 'c']]))
macd_init = h10.c.ewm(span=12, adjust=False).mean() - h10.c.ewm(span=26, adjust=False).mean()
signal_init = macd_init.ewm(span=9, adjust=False).mean()
macd = a10[1].plot(macd_init, name='MACD', lw=2)
signal = a10[1].plot(signal_init, name='Signal', lw=2)
a10[2].plot(h10.c, name='Initial close')


def mock_api(name, candle, lines, timeframe, seed, kind):
    rng = np.random.default_rng(seed)
    # Start exactly at the existing final candle's epoch, then step by timeframe.
    epoch = float((h1 if timeframe == 60 else h10).index[-1].timestamp())
    price = float(candle.data['c'].iloc[-1])
    candles = 0
    ticks = 0
    try:
        while not stop.is_set() and not fig.closed:
            # Mock one API response containing five successive price ticks.
            for _ in range(TICKS_PER_CANDLE):
                if stop.is_set() or fig.closed:
                    return
                time.sleep(0.3)  # Simulate bounded latency between quote updates

                price *= float(np.exp(rng.normal(0, 0.0015)))
                rolled = candle.update_tick(epoch, price)
                if rolled:
                    candles += 1
                    for line in lines:
                        line.shift()
                ticks += 1

                close = candle.data['c'].to_numpy()
                if kind == 'sma':
                    values = (sma20(close),)
                else:
                    values = macd_values(close)
                for line, value in zip(lines, values):
                    line.set_data([value], start=HISTORY - 1)

            if candles % 25 == 0 and ticks % (TICKS_PER_CANDLE * 25) == 0:
                print(f'{name}: candles={candles}, ticks={ticks}, epoch={epoch:.0f}, price={price:.3f}', flush=True)

            # Each producer has its OWN clock: +60s or +600s per API batch.
            epoch += timeframe
            if stop.wait(BATCH_DELAY):
                break
    except Exception:
        errors.append((name, traceback.format_exc()))
        stop.set()
        print(f'{name} producer failed:\n{errors[-1][1]}', flush=True)


threads = [
    threading.Thread(target=mock_api, args=('1m', c1, [sma], 60, 301, 'sma'), daemon=True),
    threading.Thread(target=mock_api, args=('10m', c10, [macd, signal], 600, 302, 'macd'), daemon=True),
]

print('Live test running: 2 independent views, 5 ticks per candle, concurrent producers.', flush=True)
print('Switch views with the library view-selection keys; close the figure to stop.', flush=True)

try:
    fig.show('Raycandle continuous live stress test', block=False)
    for thread in threads:
        thread.start()
    fig.wait_closed()
finally:
    stop.set()
    for thread in threads:
        thread.join(timeout=3)

if errors:
    raise RuntimeError('A producer failed; see traceback above')
print('Stopped: figure closed; both mock API producers exited.', flush=True)
