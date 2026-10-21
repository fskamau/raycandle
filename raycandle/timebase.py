"""Validation for monotonic native plot timestamps and bar intervals."""

import numpy as np
import pandas as pd


def _epochs(index, *, min_len=2):
    """Represent timestamps as seconds; numeric RangeIndex is valid too."""
    if isinstance(index,pd.DatetimeIndex):
        if index.tz is not None:
            index=index.tz_convert("UTC")
        result=index.asi8.astype(np.float64)/1e9
    else:
        result=np.asarray(index,dtype=np.float64)
    result=np.ascontiguousarray(result,dtype=np.float64)
    if result.ndim!=1 or len(result)<min_len or not np.isfinite(result).all():
        raise ValueError("Index requires at least two finite entries")
    if np.any(np.diff(result)<=0):
        raise ValueError("Index must be strictly increasing")
    return result


def _timeframe(epochs, specified):
    if specified is not None:
        if isinstance(specified, (bool, np.bool_)) or not isinstance(specified,(int,float,np.integer,np.floating)) or not np.isfinite(specified) or specified<=0 or int(specified)!=specified:
            raise ValueError("timeframe must be a positive integer number of seconds/index units")
        return int(specified)
    diffs=np.diff(epochs)
    return max(1,int(round(float(np.median(diffs)))))



def _validate_patch(existing, start, values):
    """Check that a partial timestamp update preserves strict ordering."""
    if not isinstance(start, (int, np.integer)) or isinstance(start, (bool, np.bool_)):
        raise ValueError("start must be an integer offset")
    if start < 0 or start + len(values) > len(existing):
        raise ValueError("xdata slice exceeds view length")
    if len(values) and start and values[0] <= existing[start - 1]:
        raise ValueError("xdata must remain strictly increasing at left boundary")
    end = start + len(values)
    if len(values) and end < len(existing) and values[-1] >= existing[end]:
        raise ValueError("xdata must remain strictly increasing at right boundary")
