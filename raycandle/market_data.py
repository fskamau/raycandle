from __future__ import annotations

"""Load and validate offline OHLC data before allocating native plot buffers."""
from pathlib import Path

import numpy as np
import pandas as pd

_COLUMNS = ("open", "high", "low", "close")


def read_ohlc_csv(path: str | Path) -> pd.DataFrame:
    """Read UTC ISO-8601 `timestamp,open,high,low,close` rows from a CSV.

    Returns a datetime-indexed frame with columns ``o,h,l,c``. No network or
    trading account is required. Malformed bars raise ValueError early rather
    than entering the renderer's fixed-size C allocations.
    """
    data = pd.read_csv(path)
    required = ("timestamp", *_COLUMNS)
    missing = [name for name in required if name not in data.columns]
    if missing:
        raise ValueError(f"Missing OHLC CSV columns: {missing}")
    if len(data) < 2:
        raise ValueError("At least two bars are required")

    try:
        index = pd.DatetimeIndex(pd.to_datetime(data["timestamp"], utc=True, errors="raise"))
    except (ValueError, TypeError) as exc:
        raise ValueError("Invalid timestamp in OHLC CSV") from exc
    if index.isna().any() or not index.is_monotonic_increasing or index.has_duplicates:
        raise ValueError("Timestamps must be unique, valid, and increasing")

    try:
        prices = data.loc[:, list(_COLUMNS)].apply(pd.to_numeric, errors="raise")
    except (ValueError, TypeError) as exc:
        raise ValueError("OHLC prices must be numeric") from exc
    raw = prices.to_numpy(dtype=np.float64)
    if not np.isfinite(raw).all():
        raise ValueError("OHLC prices must be finite")
    op, high, low, close = raw.T
    if (high < np.maximum(op, close)).any() or (low > np.minimum(op, close)).any():
        raise ValueError("OHLC bars violate high/low price bounds")

    result = prices.copy()
    result.columns = ("o", "h", "l", "c")
    result.index = pd.DatetimeIndex(index, name="timestamp")
    return result
