import sys
from typing import Any, NewType, Optional, Type, TypeVar, final

import numpy as np
from cffi import FFI

from .defines import *

__all__ = [
    "Color",
    "RC_Figure",
    "RC_Artist",
]
Color = NewType("Color", tuple[int, int, int, int])

_T = TypeVar("_T")


def ascii_encode(x: str) -> bytes:
    """encode `arg` into into ascii"""
    return x.encode("utf-8")


def window_not_closed(func: _T) -> _T:
    """
    make sure functions that allocate memory e.g `RC_Axes.set_title`
    should only be called when window is has been opened and also not closed since memory being referenced to
    might will be invalid once the window is closed or not be allocated at all
    """

    def wrapper(self: Type[SupportsRayCandleApi], *args: Any, **kwargs: Any):
        if self._rc_api.is_window_closed:
            exit(f"*FATAL: Call to '{func.__name__}' ignored because window is already closed*\n")  # prevent reference to dropped memory
        return func(self, *args, **kwargs)

    return wrapper


class _Api:
    lib: Any
    fig: Any
    ffi: FFI
    is_window_closed: bool = False

    def cstr(pstr: str):
        return _Api.ffi.new("char[]", ascii_encode(pstr))


class SupportsRayCandleApi:
    _rc_api: _Api


class RC_Figure(SupportsRayCandleApi):
    def __init__(self) -> None:
        self.ax: list[RC_Axes]

    def validate_xdata(artist: "RC_Artist") -> None:
        raise NotImplementedError


class RC_Artist(SupportsRayCandleApi):
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
    def set_lw(self, lw: float) -> None:
        """
        set line width
        """
        self.__artist__.thickness = lw

    @final
    @window_not_closed
    def ylim_turnoff(self) -> None:
        """
        whether the artists will be considered when ylims of its axes are calculated
        """
        self.__artist__.ylim_consider = False


class RC_Axes(SupportsRayCandleApi):
    def __init__(self, id: int) -> None:
        self._id = id
        self._hold_ref = []  # we keep artists so that they are not garage collected

    @final
    @window_not_closed
    def set_title(self, title: str) -> None:
        buf_string = self._rc_api.ffi.new("char[]", ascii_encode(title))
        self._rc_api.lib.axes_set_title(self._rc_api.fig.axes + self._id, buf_string)
