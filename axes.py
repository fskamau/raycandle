from typing import NoReturn, Type, Union, final

import numpy as np
import pandas as pd

from .artists import Line
from .bases import RC_Artist, RC_Axes, RC_Figure, window_not_closed
from .defines import *


class Axes(RC_Axes):
    def __init__(self, id: int, parent: RC_Figure) -> None:
        super().__init__(id)
        self._parent = parent
        self._rc_api = parent._rc_api

    def __str__(self) -> str:
        return f"Axes(label={self._rc_api.fig.axes[self._id].label.decode()})"

    @final
    @window_not_closed
    def v_line(self, xpos: float, **kwargs) -> None:
        """
        draws a vertical line perpendicular to `xpos`
        """
        if "line_type" in kwargs:
            raise TypeError("line type cannot be set")
        assert xpos >= 0 and xpos <= 1
        self.plot(np.float64(xpos), line_type=LINE_TYPE_V_LINE, **kwargs)

    @final
    @window_not_closed
    def h_line(self, ypos: float, **kwargs) -> None:
        """
        draws a horizontal line perpendicular to `ypos`
        """
        if "line_type" in kwargs:
            raise TypeError("line type cannot be set")
        assert ypos >= 0 and ypos <= 1
        self.plot(np.float64(ypos), line_type=LINE_TYPE_H_LINE, **kwargs)

    @window_not_closed
    def plot(self, artist: Union[Type[RC_Artist], pd.Series], **kwargs) -> Type[RC_Artist]:
        """
        set's up the artist to be plotted and returns  it.

        args
        -----
        artist: A type subclassing from RC_Artist.
            if `artist` a pandas series, then a `Line` artist will be created for the series
        """
        if not issubclass(type(artist), RC_Artist):
            if isinstance(artist, pd.Series):
                artist = Line(artist, **kwargs)
            elif isinstance(artist, np.float64):
                artist = Line(pd.Series([artist]), **kwargs)
            elif isinstance(artist, (tuple, list)):
                artist = Line(pd.Series(artist), **kwargs)
            else:
                raise Exception(f"{type(artist)} may need to subclass {RC_Artist}")
        if hasattr(artist, "xdata"):
            self._parent.validate_xdata(artist)
        artist._rc_api = self._rc_api
        args = artist._get_create_args()
        artist.__artist__ = self._rc_api.lib.create_artist(self._rc_api.fig.axes + self._id, *args)
        self._hold_ref.append(artist)
        return artist

    @final
    @window_not_closed
    def set_forecolor(self, color: tuple[int, int, int, int]) -> None:
        self._rc_api.fig.axes[self._id].facecolor = color

    @final
    @window_not_closed
    def set_ylim(self, ylim: tuple[float, float]) -> None:
        self._rc_api.lib.axes_set_ylim(self._rc_api.fig.axes + self._id, ylim)

    @final
    @window_not_closed
    def show_cursor(self) -> None:
        self._rc_api.fig.axes[self._id].show_cursor = True

    @final
    @window_not_closed
    def set_xlim(self, *args, **kwargs) -> NoReturn:
        raise NotImplementedError("all axes share same x-axis for now. to update all axes xlim, update from a point using Figure.update_from_position")

    @final
    @window_not_closed
    def grid(self, xlen: int = 3, ylen: int = 3, color=(0, 0, 0, 255), lw=0.3, **kwargs) -> None:
        [
            [func(0 if x == 0 else float(x) / len_, color=color, lw=lw, **kwargs) for x in range(len_)]
            for (func, len_) in ((self.h_line, xlen), (self.v_line, ylen))
        ]

    @final
    @window_not_closed
    def show_legend(self, legend_position: LegendPosition = LegendPosition.BOTTOM_LEFT) -> None:
        self._rc_api.lib.axes_show_legend(self._rc_api.fig.axes + self._id, legend_position)
