from typing import Any, Optional

import numpy as np
import pandas as pd
from typing_extensions import override

from .bases import RC_Artist, ascii_encode, window_not_closed
from .defines import *

__all__ = ["Line", "Candle"]


class Line(RC_Artist):
    def __init__(
        self,
        line: pd.Series,
        lw: float = 1.0,
        color: Optional[tuple[int]] = None,
        line_type: LineType = LineType.S_LINE,
        label: str = None,
        label_from_data: bool = False,
    ):
        self.ydata = line.to_numpy(dtype=np.float64)
        if line_type == LineType.S_LINE:
            self.xdata = line.index.to_numpy(dtype=np.float64)
        self.line_type = line_type
        if label is None and label_from_data:
            label = str(line.name)
        self.label = label
        self.thick = lw
        self.__data_names__ = line.name
        self.color = np.array(color) if color is not None else None
        if self.color is not None:
            self.color = self.color.flatten("C").astype(np.int8)
            if len(self.color) != 4:
                raise Exception("color must be tuple of 4 short ints(R,G,B,A)")

    @override
    def _get_create_args(self) -> tuple[Any]:
        self.config = self._rc_api.ffi.new("LineData*")
        self.config.line_type = self.line_type
        self.config.data = self._rc_api.ffi.NULL
        self._ydata_pointer = self._rc_api.ffi.cast("double*", self.ydata.ctypes.data)
        self._label = self._rc_api.cstr(self.label) if self.label is not None else self._rc_api.ffi.NULL
        gdata = {"cols": 1, "ydata": self._ydata_pointer, "label": self._label}
        self._color_ptr = self._rc_api.ffi.cast("Rc_Color*", self.color.ctypes.data if self.color is not None else self._rc_api.ffi.NULL)
        return (ArtistType.LINE, gdata, (np.nanmin(self.ydata), np.nanmax(self.ydata)), self.thick, self._color_ptr, self.config)

    @override
    @window_not_closed
    def set_data(self, data: pd.Series) -> None:
        if not hasattr(self, "xdata"):
            raise Exception("cannot set data for non segmented line")
        if len(data) != len(self.xdata):
            raise Exception("length mismatch")
        self.ydata = data.to_numpy(dtype=np.float64)
        self.__artist__.gdata.ydata = self._rc_api.ffi.cast("double*", self.ydata.ctypes.data)


class Candle(RC_Artist):
    GREEN = (44, 160, 44, 255)
    RED = (255, 0, 0, 255)

    def __init__(self, df: pd.DataFrame, lw: float = 1.0, colors: tuple[int] = (RED, GREEN), label: str = None, label_from_data: bool = False):
        if (len(df.columns)) != 4:
            raise Exception("len of candle columns should be 4")
        self.__data_names__ = df.columns
        self.xdata = df.index.to_numpy(dtype=np.float64)
        self.ydata = df.to_numpy(dtype=np.float64).flatten(order="F")
        if label_from_data and label is None:
            label = f"Candlestick({','.join([str(x) for x in df.columns])})"
        self.label = label
        self.color = np.array(colors) if colors is not None else None
        if self.color is not None:
            self.color = self.color.flatten("C").astype(np.int8)
            if len(self.color) != 8:
                raise Exception("colors must be of shape 2x4")
        self.thick = lw

    @override
    def _get_create_args(self) -> tuple[Any]:
        self._label = self._rc_api.cstr(self.label) if self.label is not None else self._rc_api.ffi.NULL
        self._ydata_pointer = self._rc_api.ffi.cast("double*", self.ydata.ctypes.data)
        gdata = (
            4,
            self._ydata_pointer,
            self._label,
        )
        self._color_pointer = self._rc_api.ffi.cast("Rc_Color*", self.color.ctypes.data if self.color is not None else self._rc_api.ffi.NULL)
        return (
            ArtistType.CANDLE,
            gdata,
            [np.nanmin(self.ydata), np.nanmax(self.ydata)],
            self.thick,
            self._color_pointer,
            self._rc_api.ffi.NULL,
        )

    @override
    def set_data(self, data: pd.DataFrame) -> None:
        if len(data) != len(self.xdata):
            raise Exception("length mismatch")
        if (len(data.columns)) != 4:
            raise Exception("len of candle columns should be 4")
        self.ydata = data.to_numpy(dtype=np.float64).flatten(order="F")
        self.__artist__.gdata.ydata = self._rc_api.ffi.cast("double*", self.ydata.ctypes.data)
