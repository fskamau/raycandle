import itertools
import threading
import warnings
from typing import Any, NoReturn, Type
import os 
import cffi
import numpy as np
import pandas as pd

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
        visible_data: int = 0,
        update_len: int = 0,
        fps: int = 30,
        font_size: int = 20,
        font_spacing: int = 2,
    ):
        super().__init__()
        self._xdata: np.array = None
        self._pxdata: Any = None
        self._axes: tuple[Type[RC_Axes]]
        self.visible_data, self.update_len = visible_data, update_len
        rows, cols, labels, axes_skels = self._generate_axes_skeleton(fig_skel)
        self._load_lib()
        self._rc_api.fig = self._rc_api.lib.create_figure(
            fig_size,
            self._rc_api.cstr(window_title) if window_title is not None else self._rc_api.ffi.NULL,
            fig_background,
            len(labels),
            rows,
            cols,
            axes_skels,
            [ascii_encode(x) for x in labels],
            border_percentage,
            fps,
            font_size,
            font_spacing,
            self._rc_api.cstr(os.path.join(FPATH,FONT_PATH))
        )
        self._xformatter_type, self._xlim_format = FormatterType.TIME, self._rc_api.cstr("[%Y-%m-%d %H:%M:%S]")
        self.axes: list[Axes]
        self.ax: list[Axes]
        self._init()

    @staticmethod
    def _generate_axes_skeleton(f):
        tos = set(" \t")
        k = [list(iter(x)) for x in f.splitlines()]
        c = [x for x in k if len(x) != 0 and all([x.count(o) != len(x) for o in " \t"]) and set(x) != tos]
        l = [len(x) for x in c]
        if len(l) == 0:
            raise Exception("no characters on the string")
        if l.count(l[0]) != len(l):
            raise Exception(f"lengths of linesplit axes-mosaic do not match {[''.join(x) for x in c]} has lengths {l}")
        Rows = len(labels := ["".join(x) for x in c])
        labels_, tosl, Cols = ["", []], [[], []], sum([labels[0].count(x) for x in set(labels[0]).difference(tos)])
        for row in range(len(labels)):
            for col in range(len(labels[0])):
                char = labels[row][col]
                if set(char).intersection(tos):
                    if row == 0:
                        tosl[0].append(col)
                        tosl[1].append(0)
                    else:
                        try:
                            i = tosl[0].index(col)
                            if tosl[1][i] == row - 1:
                                tosl[1][i] += 1
                            else:
                                raise ValueError
                        except ValueError:
                            raise Exception(f"separator '{char}' must be along all rows of col {col}")
                    continue
                if char not in labels_[0]:
                    if char == "\0":
                        raise Exception("labels may not contain null terminator '\\0'")
                    labels_[0] += char
                    labels_[1].append([col, row, 0, 0])
                else:
                    pl = labels_[1][labels_[0].index(char)]
                    if row == pl[1]:
                        if col - (pl[0] + pl[2]) == 1:
                            pl[2] += 1
                        else:
                            raise Exception(f"label '{char}' has jumped from col {pl[0]+pl[2]} to {col} across row {row}")
                    else:  # exit early
                        if labels[row].count(char) > 0:
                            if labels[row][pl[0] : pl[2] + pl[0] + 1].count(char) != pl[2] + 1:
                                raise Exception(
                                    f"char '{char}' occurred first in row {pl[1]} cols [{pl[0]}:{pl[2]+pl[0]+1}]=[{labels[pl[1]][pl[0]:pl[2]+pl[0]+1]}] but exact same cols in row {row} have [{labels[row][pl[0]:pl[2]+pl[0]+1]}]"
                                )
                            if labels[row].count(char) > labels[pl[1]].count(char):
                                raise Exception(f"excess char '{char}' in row {row} as compared to first occurrent row {pl[1]};")
                    if col == pl[0]:
                        if row - (pl[1] + pl[3]) == 1:
                            pl[3] += 1
                        else:
                            raise Exception(f"label '{char}' has jumped from row {pl[1]+pl[3]} to row {row} along col {col}")
        m = min([x[0] for x in labels_[1]])
        skels = []
        for x in [[s[0] - m, s[1], s[2] + 1, s[3] + 1] for s in labels_[1]]:
            skels = [*skels, *x]
        return Rows, Cols, [x for x in labels_[0]], skels

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
        return self._rc_api.fig.dragger.len_data
        
    def _init(self) -> None:
        self.axes = self.ax = [Axes(i, self) for i in range(self._rc_api.fig.axes_len)]

    @window_not_closed
    def _show(self) -> None:        
        self._rc_api.is_window_closed = False
        self._rc_api.lib.show(self._rc_api.fig)
        self._rc_api.is_window_closed = True
        # clean afterwards
        self._rc_api.lib.cm_free_all_()

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
            return
        self._show()
        self._rc_api.lib.cm_free_all_()

    @window_not_closed
    def show_cursors(self) -> None:
        self._rc_api.fig.show_cursors = True

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
        self._rc_api.lib.update_from_position(self._rc_api.fig.dragger.cur_position, self._rc_api.fig)

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
            dragger = {
                "len_data": len(self._xdata),
                "visible_data": self.visible_data,
                "update_len": self.update_len,
                "xdata": self._pxdata,
                "xdata_shared": self._rc_api.ffi.NULL,
                "xlim_format": self._xlim_format,
                "xformatter": self._xformatter_type,
                "timeframe": timeframe,
            }
            self._rc_api.lib.set_dragger(self._rc_api.fig, dragger)
        else:
            if not np.all(self._xdata == artist.xdata):
                raise NotImplementedError("got different xdata; all artist must share x-axis")

    @window_not_closed
    def set_xformatter(self, xformatter_type: FormatterType, formatter_str=None):
        if xformatter_type not in FormatterType:
            raise Exception(f"unknown type {type(xformatter_type)}")
        if xformatter_type == FormatterType.NULL:
            if formatter_str is not None:
                raise Exception("NULL formatter expects  not formatter str")
            fstr = self._rc_api.ffi.NULL
        else:
            if formatter_str is None:
                raise Exception("non-NULL formatter cannot have NULL str")
            fstr = self._rc_api.cstr(formatter_str)
        if self._rc_api.fig.has_dragger:
            self._rc_api.lib.string_clear(self._rc_api.fig.dragger.xlim_format)
            if formatter_str is not None:
                self._rc_api.lib.string_append(self._rc_api.fig.dragger.xlim_format, self._rc_api.cstr("%s"), fstr)
            self._rc_api.fig.dragger.xformatter = xformatter_type
        else:
            self._xlim_format, self._xformatter_type = fstr, xformatter_type

    @window_not_closed
    def set_timeframe(self, timeframe: int) -> None:
        self._rc_api.lib.update_timeframe(self._rc_api.fig, timeframe)

    @window_not_closed
    def show_legend(self) -> None:
        [x.show_legend() for x in self.ax]
