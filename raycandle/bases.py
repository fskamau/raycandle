import sys
from typing import Any, NewType, Optional, Type, TypeVar, final

import numpy as np
from cffi import FFI

from .defines import *

__all__ = [
    "Artist",
    "Color",
]

_T = TypeVar("_T")


def window_not_closed(func: _T) -> _T:
    """
    make sure functions that allocate memory e.g `RC_Axes.set_title`
    should only be called when window is has been opened and also not closed since memory being referenced to
    might will be invalid once the window is closed or not be allocated at all
    """

    def wrapper(self: Type[RApi], *args: Any, **kwargs: Any):
        if self._api.closed:
            raise SystemExit(f"Call to '{func.__name__}' ignored because window is already closed*\n")  # prevent reference to dropped memory
        return func(self, *args, **kwargs)

    return wrapper


class RApi:
    ARENA_MAX_SIZE = 1024**2
    def __init__(self, lib: Any, fig: Any, ffi: FFI, figure: Figure):
        self.fig = fig
        self.ffi = ffi
        self.lib = lib
        self.null = self.ffi.NULL
        self.closed = False
        self.figure = figure
        self.total_len=0
        self.base_view_set=False
        self.arena=lib.arena_create(RApi.ARENA_MAX_SIZE)

    def cstr(self, s: Optional[str]):
        return self.null if s is None else self.ffi.new("char[]", s.encode("utf-8"))


class Artist(RApi):
    def __init__(self):
        self.color: Optional[tuple[int, int, int, int]]
        self.xdata: np.array  # x data
        self.ydata: np.array  # A shared reference
        self.__artist__: FFI.CData  # actual C artist
        self.__data_names__: Union[list[str], str]  # data name(s) of Series/DataFrame

    def _get_create_args(self) -> tuple[Any]:
        """
        returns the @self._api.lib.create_artist arguments
        """
        raise NotImplementedError

    @window_not_closed
    def set_data(self, data: Any) -> None:
        """
        updates the artist data.
        NOTE:
        ------
        Care should be taken as `RC_Artist.ydata` is a shared reference
        If need be to change `RC_Artist.ydata` one could:
            1. copy the new data into `RC_Artist.ydata` using a loop. New data
        should have the same length
            2. Drop the reference  by assigning `RC_Artist` to some new numpy float64 array
        (which must be C_CONTINGUOUS) and then cast it's ctypes.data to double* and assign  that
        pointer to the appropriate value
        """
        raise NotImplementedError


    @final
    @window_not_closed
    def ylim_turnoff(self) -> None:
        """
        whether the artists will be considered when ylims of its axes are calculated
        """
        self.__artist__.is_limitable = False
        return self



    def fade(self, alpha:float)->'self':
        if alpha<=0.0 or  alpha>1.0:raise ValueError("invalid fade value")
        self.__artist__.colors[0].a=int(alpha*255)
        return self

    def __repr__(self)->str:
        return f"Artist(label='{'' if self.__artist__.label==self._api.null else self._api.ffi.string(self.__artist__.label).decode()}')"

    
class Color:
    def __init__(self, r: int, g: int, b: int, a: int = 0xFF):
        self.r = max(0, min(255, r))
        self.g = max(0, min(255, g))
        self.b = max(0, min(255, b))
        self.a = max(0, min(255, a))
        
    @classmethod
    def fromhex(cls, hex_value: int):
        """Parses an integer hex value (0xRRGGBB or 0xRRGGBBAA)."""
        if hex_value <= 0xFFFFFF:
            hex_value = (hex_value << 8) | 0xFF
    
        r = (hex_value >> 24) & 0xFF
        g = (hex_value >> 16) & 0xFF
        b = (hex_value >> 8) & 0xFF
        a = hex_value & 0xFF
            
        return cls(r, g, b, a)

    def fade(self, factor: float):
        factor = max(0.0, min(1.0, factor))
        new_a = int(self.a * factor)
        return Color(self.r, self.g, self.b, new_a)

    def __repr__(self):
        return f"Color({self.r:3d}, {self.g:3d}, {self.b:3d}, {self.a:3d})"
    
    def __iter__(self):
        yield self.r
        yield self.g
        yield self.b
        yield self.a
