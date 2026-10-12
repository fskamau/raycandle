from typing import NoReturn, Type, Union, final

import numpy as np
import pandas as pd

from .artists import Line
from .bases import Artist, window_not_closed
from .defines import *


class Axes:
    def __init__(self, index: int, api: Api) -> None:
        self._api = api
        self._axes = api.fig.axes + index
        self._index = index
        self._hold_ref = []  # we keep artists so that they are not garage collected

    def __repr__(self) -> str:
        return f"Axes[{self._index}])"

    @window_not_closed
    def plot(self, artist: Union[Type[Artist], pd.Series], **kwargs) -> Type[RC_Artist]:
        """
        set's up the artist to be plotted and returns  it.

        args
        -----
        artist: A type subclassing from RC_Artist.
            if `artist` a pandas series, then a `Line` artist will be created for the series
        """
        if not issubclass(type(artist), Artist):
            if isinstance(artist, pd.Series):
                artist = Line(artist, **kwargs)
            elif isinstance(artist, np.float64):
                artist = Line(pd.Series([artist]), **kwargs)
            elif isinstance(artist, (tuple, list)):
                artist = Line(pd.Series(artist), **kwargs)
            else:
                raise Exception(f"{type(artist)} may need to subclass {Artist}")
        artist._api = self._api
        args = artist._get_create_args()
        artist.__artist__ = self._api.lib.artist_create(self._axes, *args)
        self._hold_ref.append(artist)
        return artist

    @final
    @window_not_closed
    def set_forecolor(self, color: tuple[int, int, int, int]) -> None:
        self._api.fig.axes[self._id].facecolor = color

    @final
    @window_not_closed
    def set_ylim(self, vmin:float, vmax:float) -> None:
        self._api.lib.axes_set_ylim(self._axes , vmin,vmax)
        return self

    @final
    @window_not_closed
    def show_cursor(self) -> None:
        self._api.fig.axes[self._id].show_cursor = True

    @final
    @window_not_closed
    def set_xlim(self, *args, **kwargs) -> NoReturn:
        raise NotImplementedError("all axes share same x-axis for now. to update all axes xlim, update from a point using Figure.update_from_position")

    @final
    @window_not_closed
    def grid(self, xlen: int = 3, ylen: int = 3, color=(0, 0, 0, 255), lw=0.3, **kwargs) -> None:
        [[func(0 if x == 0 else float(x) / len_, color=color, lw=lw, **kwargs) for x in range(len_)] for (func, len_) in ((self.h_line, xlen), (self.v_line, ylen))]

    @final
    @window_not_closed
    def show_legend(self, legend_position: LegendPosition = LegendPosition.BOTTOM_LEFT) -> None:
        self._api.lib.axes_show_legend(self._axes, legend_position)

    @final
    @window_not_closed
    def set_yformat(self, format_:str):
        self._api.lib. axes_set_yformatter(self._axes,format_.encode())
