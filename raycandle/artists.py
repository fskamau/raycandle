"""Fixed-size C-owned arrays with NumPy/pandas views (no per-tick copies)."""
from __future__ import annotations

import numpy as np
import pandas as pd

from .bases import Artist, Color, window_not_closed
from .defines import ArtistType

__all__ = ["Line", "Candle"]


def _as_series(data):
    if isinstance(data, pd.Series):
        return data
    if isinstance(data, (list, tuple, np.ndarray)):
        return pd.Series(data)
    raise TypeError("Expected pandas Series or a 1-D list/array")


class Line(Artist):
    def __init__(self, data, *, name=None, label=None, lw=1.0, thick=None,
                 color=None, is_relative=False, is_splined=True, label_from_data=True):
        data = _as_series(data)
        self.xdata = data.index
        self._initial = np.ascontiguousarray(data.to_numpy(dtype=np.float32), dtype=np.float32)
        self.is_splined = bool(is_splined)
        self.is_relative = bool(is_relative)
        if not self.is_splined and len(data) != 4:
            raise ValueError("Non-splined lines require exactly four floats: x0,x1,y0,y1")
        self.label = label if label is not None else name
        if self.label is None and label_from_data:
            self.label = str(data.name) if data.name is not None else None
        self.__data_names__ = data.name
        self.thick = float(lw if thick is None else thick)
        self.color = color

    def _create(self, api, axes):
        ffi = api.ffi
        spec = ffi.new("LineData*", {
            "is_splined": int(self.is_splined),
            "data": ffi.cast("float*", self._initial.ctypes.data),
        })
        rgba = ffi.NULL if self.color is None else ffi.new("RColor*", tuple(self.color))
        label = api.cstr(self.label)
        # Source arrays + CFFI temporaries must survive the artist_create call.
        ptr = api.lib.artist_create(axes, int(ArtistType.LINE.value),
                                    self.thick, rgba, label, spec)
        self._bind(api, ptr)

    def _bind(self, api, ptr):
        self._api, self.__artist__ = api, ptr
        count = int(api.lib.artist_get_data_len(ptr))
        self.ydata = np.frombuffer(api.ffi.buffer(api.lib.artist_get_data_ptr(ptr), 4*count), dtype=np.float32)
        self.data = pd.Series(self.ydata, index=self.xdata if self.is_splined else None,
                              name=self.label, copy=False)
        del self._initial

    @window_not_closed
    def set_data(self, data, start=0):
        """Write only the supplied slice into the native buffer."""
        raw = np.asarray(data, dtype=np.float32)
        if raw.ndim != 1 or not len(raw) or start < 0 or start+len(raw) > len(self.ydata):
            raise ValueError("Line data must fit its original fixed capacity")
        raw=np.ascontiguousarray(raw)
        self._api.lib.artist_line_set_data(self.__artist__,
            self._api.ffi.cast("float*",raw.ctypes.data),start,len(raw))
        return self

    update_tail = set_data

    @window_not_closed
    def shift(self, count=1):
        """Drop `count` oldest slots, left-shift in C, fill the tail with NaN.

        After a candle rollover, call shift() and set_data() for recomputed
        indicator values. Do not need to recopy historical unchanged values.
        """
        if not self.is_splined or not 1<=count<=len(self.ydata):
            raise ValueError("shift count must be in 1..len for splined lines")
        self._api.lib.artist_line_shift(self.__artist__,int(count))
        return self


class Candle(Artist):
    GREEN = Color.fromhex(0x079981ff)
    RED = Color.fromhex(0xf23645ff)

    def __init__(self, df, *, lw=1.0, thick=None, bull=GREEN, bear=RED,
                 label=None, name=None, label_from_data=True):
        if not isinstance(df, pd.DataFrame) or df.shape[1] != 4:
            raise ValueError("Candle needs a DataFrame with four columns in O,H,L,C order")
        self.__data_names__ = tuple(df.columns)
        self.xdata = df.index
        self._initial = np.ascontiguousarray(df.to_numpy(dtype=np.float32).T).ravel()
        self.label = label if label is not None else name
        if self.label is None and label_from_data:
            self.label = "Candlestick(" + ",".join(map(str,df.columns)) + ")"
        self.thick = float(lw if thick is None else thick)
        self.color = (tuple(bull),tuple(bear))

    def _create(self, api, axes):
        ffi = api.ffi
        rgba=ffi.new("RColor[2]", self.color)
        label=api.cstr(self.label)
        ptr=api.lib.artist_create(axes, int(ArtistType.CANDLE.value),
                                  self.thick, rgba, label,
                                  ffi.cast("void*", self._initial.ctypes.data))
        self._api, self.__artist__ = api, ptr
        n=int(api.lib.artist_get_data_len(ptr))//4
        self.ydata=np.frombuffer(ffi.buffer(api.lib.artist_get_data_ptr(ptr), n*4*4), dtype=np.float32)
        # Column-major storage: pandas/NumPy share the *C-owned* bytes.
        matrix=self.ydata.reshape(4,n).T
        self.data=pd.DataFrame(matrix, columns=self.__data_names__, index=self.xdata, copy=False)
        del self._initial

    @window_not_closed
    def set_data(self, data, start=0):
        """Write changed O/H/L/C rows only (no full-frame copy)."""
        if not isinstance(data, pd.DataFrame) or tuple(data.columns)!=self.__data_names__:
            raise ValueError("Candle columns and order must match initial O,H,L,C")
        n=len(self.ydata)//4
        count=len(data)
        if not count or start<0 or start+count>n:
            raise ValueError("Candle slice exceeds fixed capacity")
        # Only pack the changed tail (4 * count floats), not the full frame.
        packed=np.empty(count*4,dtype=np.float32)
        for i,col in enumerate(self.__data_names__):
            packed[i*count:(i+1)*count]=data[col].to_numpy(dtype=np.float32,copy=False)
        self._api.lib.artist_candle_set_data(self.__artist__,
            self._api.ffi.cast("float*",packed.ctypes.data),start,count)
        return self

    update_tail=set_data

    @window_not_closed
    def update_tick(self, epoch, price):
        """Update last OHLC bar in C; returns True when a new bar rolls over."""
        rc=self._api.lib.artist_candle_update_tick(self.__artist__,float(epoch),float(price))
        if rc<0:
            raise ValueError("Stale/non-finite tick, or uninitialized candle")
        return bool(rc)
