from typing import Any, Optional

import numpy as np
import pandas as pd
from typing_extensions import override

from .bases import Artist, window_not_closed, Color
from .defines import *

__all__ = ["Line", "Candle"]

class Line(Artist):
    def __init__(self, data: pd.Series, thick: float = 1.0, color: Optional[Color]=None,is_relative=False,is_splined=True, label: str = None, label_from_data: bool = False):
        if is_relative:
            if(len(data)!=4):raise Exception
        if is_splined:
            self.xdata=data.index
        self.is_relative=is_relative
        self.is_splined=is_splined
        self.ydata = data.to_numpy(dtype=np.float64)
        if label is None and label_from_data:
            label = str(line.name)
        self.label = label
        self.thick = thick
        self.__data_names__ = data.name
        self.color = tuple(color) if color is not None else None

        
    @override
    def _get_create_args(self) -> tuple[Any]:
        line_data = self._api.ffi.new("LineData*")
        line_data.is_splined = self.is_splined
        line_data.is_relative = self.is_relative
        line_data.data=self._pdata = self._api.ffi.cast("double*",self.ydata.ctypes.data)
        return (
            ArtistType.LINE,
            self.thick,                                   
            self._api.ffi.new("RColor*",self.color) if self.color is not None else self._api.ffi.NULL    ,
            self._api.cstr(self.label),
            line_data,
        )

    @override
    @window_not_closed
    def set_data(self, data: pd.Series) -> None:
        self.ydata = data.to_numpy(dtype=np.float64)
        self.__artist__.gdata.ydata = self._api.ffi.cast("double*", self.ydata.ctypes.data)

    @override
    def set_data(self, data: pd.Series, start:int=0) -> None:
        """
        copy `data` to the internal buffer from `start` 
        """
        x=len(data)
        df=data.to_numpy(dtype=np.float64)
        self._api.lib.artist_line_set_data(self.__artist__, self._api.ffi.cast('double*',df.ctypes.data), start ,x)
        
class Candle(Artist):
    GREEN = Color.fromhex(0x079981ff)
    RED =  Color.fromhex(0xf23645ff)

    def __init__(self, df: pd.DataFrame, thick: float = 1.0, bull:Color=GREEN,bear:Color=RED,label:Optional[st]=None, label_from_data=True):
        if (len(df.columns)) != 4:
            raise Exception("len of candle columns should be 4")
        self.__data_names__ = df.columns
        self.xdata = df.index
        self.ydata = df.to_numpy(dtype=np.float64).flatten(order="F")
        if label_from_data and label is None:
            label = f"Candlestick({','.join([str(x) for x in df.columns])})"
        self.label = label
        self.color=[tuple(x) for x in [bull,bear]]
        self.thick = thick

    
    @override
    def _get_create_args(self) -> tuple[Any]:
        return (
            ArtistType.CANDLE,
            self.thick,
            self._api.ffi.new("RColor[2]",self.color) if self.color is not None else self._api.ffi.NULL,
            self._api.cstr(self.label),
            self._api.ffi.cast("double*", self.ydata.ctypes.data),
        )

    @override
    def set_data(self, data: pd.DataFrame, start:int=0) -> None:
        """
        copy `data` to the internal buffer from `start` 
        """
        x=len(data)
        df=data.to_numpy(dtype=np.float64).flatten(order="F")
        self._api.lib.artist_candle_set_data(self.__artist__, self._api.ffi.cast('double*',df.ctypes.data), start ,x)
