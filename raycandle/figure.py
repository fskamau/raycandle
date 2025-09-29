import itertools
import threading
import warnings
from typing import Any, NoReturn, Type
import os 
import cffi
import numpy as np
import pandas as pd
import signal
from .axes import Axes
from .bases import (RC_Artist, RC_Axes, RC_Figure, _Api, ascii_encode,
                    window_not_closed)
from .defines import *
from .exceptions import *

LIB_NAME="libraycandle.so.1"
CDEF_NAME="raycandle_for_cffi.h"
FONT_PATH="fonts/font.ttf"
FPATH=os.path.dirname(__file__)

class Collector:
    def __init__(self, fig: RC_Figure, *args: RC_Artist):
        self._artists, self.__data_names__ = [], set()
        if not isinstance(fig, Figure):
            raise TypeError(f"expected a Figure instance got {type(fig)}")
        self._fig = fig
        self._set_dict([args])

    def __getitem__(self,index):
        return self._artists[index]
    
    def _set_dict(self, a):
        if issubclass(type(a), RC_Artist):
            if not hasattr(a, "__data_names__"):
                raise NotImplementedError(f"artist {a} has no attribute '__data_names__'")
            self._artists.append(a)
        elif isinstance(a, (tuple, list)):
            tuple(map(self._set_dict, a))
        else:
            raise NotImplementedError(f"{type(a)} is not an Artist")

    def update(self, df: pd.DataFrame):
        if len(df) != self._fig.len_data:
            raise RuntimeError(f"length mismatch; prev {self._fig.len_data} new {len(df)}")
        self._fig.set_xdata(df.index)
        if diff := self.__data_names__.difference(df.columns):
            raise ValueError(f"missing column names {diff}")
        tuple(map(lambda x: x.set_data(df[x.__data_names__]), self._artists))
        self._fig.update()


class Figure(RC_Figure):
    def __init__(
        self,
        fig_skel: str = "a",
        fig_size: tuple[int, int] = [1280, 720],
        window_title: str = None,
        fig_background=(255, 255, 255, 255),
        border_percentage: float = 1.0 / 100,
        fps: int = 45,
        font_size: int = 20,
        font_spacing: int = 2,
    ):
        super().__init__()
        self._xdata: np.array = None
        self._pxdata: Any = None
        self._axes: tuple[Type[RC_Axes]]
        self._load_lib()
        self._rc_api.fig = self._rc_api.lib.create_figure(
            self._rc_api.cstr(fig_skel.replace("\n"," ")),
            fig_size,
            self._rc_api.cstr(window_title) if window_title is not None else self._rc_api.ffi.NULL,
            fig_background,
            border_percentage,
            fps,
            font_size,
            font_spacing,
            self._rc_api.cstr(os.path.join(FPATH,FONT_PATH)))
        self._xformatter_type, self._xlim_format = FormatterType.TIME, self._rc_api.cstr("[%Y-%m-%d %H:%M:%S]")
        self.axes: list[Axes]
        self.ax: list[Axes]
        self._init()

    def _load_lib(self) -> None:
        self._rc_api = _Api
        self._rc_api.is_window_closed = False
        self._rc_api.ffi = cffi.FFI()
        self._rc_api.lib = self._rc_api.ffi.dlopen(os.path.join(FPATH,LIB_NAME))
        self._rc_api.ffi.cdef(open(os.path.join(FPATH,CDEF_NAME)).read())

    @property
    def is_window_closed(self) -> bool:
        return self._rc_api.is_window_closed

    @property
    @window_not_closed
    def len_data(self) -> int:
        return self._rc_api.fig.dragger._len
        
    def _init(self) -> None:
        self.axes = self.ax = [Axes(i, self) for i in range(self._rc_api.fig.axes_len)]

    @window_not_closed
    def _show(self) -> None:        
        self._rc_api.is_window_closed = False
        self._rc_api.lib.show(self._rc_api.fig)
        self._rc_api.is_window_closed = True
        # clean afterwards
        self._rc_api.lib.cm_free_all_()

    def _wait_init(self)->None:
        """
        return when figure is allocated in memory to prevent refrencing to NULL when
        calling from another thread
        """
        while 1:
            if self._rc_api.fig.initialized:return 
    @window_not_closed
    def show(self, block: bool = True) -> None:
        """
        starts a game loop and draws artists on the screen.

        args
        ----
        block: whether to use a separate thead to run the game loop.

        if False, this function will return immediately otherwise it will block until the window is closed

        NOTE:
        -----
        sending SIGINT will be ignored if `show` runs on the main thread i;e Ctrl-C won't work
        """
        if not block:
            threading.Thread(target=self._show).start()
            return self._wait_init()
        self._show()
        self._rc_api.lib.cm_free_all_()

    @window_not_closed
    def show_cursors(self) -> None:
        self._rc_api.fig.show_cursors=True

    @window_not_closed
    def set_xdata(self, xdata: pd.Index) -> None:
        """
        sets the shared x-axis data. All artists plotted after this method is called
        shall be treated as using `xdata` for their xdata
        """
        if len(self._xdata) != len(xdata):
            raise Exception("length mismatch")
        self._xdata[0:] = xdata.astype(np.float64)

    @window_not_closed
    def update_from_position(self, far_right_position: int):
        """
        sets `new_position` as the right most  data position
        and updates the figure.
        """
        self._rc_api.lib.update_from_position(far_right_position, self._rc_api.fig)

    @window_not_closed
    def update(self):
        """
        just updates the figure by recomputing all limits and adjusting data within the limit
        """
        self._rc_api.lib.update_from_position(self._rc_api.fig.dragger.start, self._rc_api.fig)

    @window_not_closed
    def set_title(self, title: str) -> None:
        self._rc_api.lib.figure_set_title(self._rc_api.fig, self._rc_api.cstr(title))

    @window_not_closed
    def validate_xdata(self, artist: RC_Artist):
        if not hasattr(artist, "xdata"):
            raise AttributeError(f"expects an attribute `xdata` to be set for {type(artist)}")
        if self._xdata is None:
            timeframe = np.bincount([x for x in np.diff(artist.xdata) if not np.isnan(x)]).argmax()
            if len(pd.Series(artist.xdata).dropna().diff().iloc[1:].drop_duplicates()) != 1:
                warnings.warn(f"index spacing is not equal, the most occurrent spacing ({timeframe}) will be applied", RuntimeWarning)
            self._xdata = artist.xdata.copy()
            self._pxdata = self._rc_api.ffi.cast("double*", self._xdata.ctypes.data)
            self._rc_api.lib.set_dragger(self._rc_api.fig, len(self._xdata),timeframe,self._pxdata,self._xformatter_type,self._xlim_format)
        else:
            if not np.all(self._xdata == artist.xdata):
                raise NotImplementedError("got different xdata; all artist must share x-axis")

    @window_not_closed
    def set_timeframe(self, timeframe: int) -> None:
        self._rc_api.lib.update_timeframe(self._rc_api.fig, timeframe)

    @window_not_closed
    def show_legend(self) -> None:
        [x.show_legend() for x in self.ax]

