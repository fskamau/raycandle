"""Each Axes belongs to one independent Figure view."""
from __future__ import annotations
import numpy as np
import pandas as pd

from .artists import Line, Candle
from .bases import Artist, window_not_closed
from .defines import LegendPosition


class Axes:
    def __init__(self, view, index):
        self._view=view
        self._index=index
        self._api=view._figure._api
        self._axes=self._api.lib.figure_get_axes(self._api.fig,index)
        self._hold_ref=[]

    def _select(self):
        self._view.select()

    @window_not_closed
    def plot(self, artist, **kwargs):
        if not isinstance(artist,Artist):
            if isinstance(artist,pd.DataFrame):
                artist=Candle(artist,**kwargs)
            else:
                artist=Line(artist,**kwargs)
        elif kwargs:
            raise TypeError("Pass styling options to the Artist constructor")
        if self._view._figure._show_started:
            raise RuntimeError("Plot artists before show(); only data updates run live")
        if hasattr(artist,"__artist__"):
            raise ValueError("An Artist instance can only be added once")
        self._view._ensure_data(artist)
        self._select()
        artist._create(self._api,self._axes)
        artist._view_id=self._view.id
        self._hold_ref.append(artist)
        self._api.lib.figure_invalidate(self._api.fig)
        return artist

    @window_not_closed
    def set_ylim(self, vmin,vmax):
        self._select()
        self._api.lib.axes_set_ylim(self._axes,float(vmin),float(vmax))
        self._api.lib.figure_invalidate(self._api.fig)
        return self

    @window_not_closed
    def show_legend(self,position=LegendPosition.BOTTOM_LEFT):
        self._select()
        self._api.lib.axes_show_legend(self._axes,int(position.value))
        self._api.lib.figure_invalidate(self._api.fig)
        return self

    @window_not_closed
    def set_yformat(self, fmt):
        self._select()
        self._api.lib.axes_set_yformatter(self._axes,self._api.cstr(fmt))
        self._api.lib.figure_invalidate(self._api.fig)
        return self

    def __repr__(self):
        return f"Axes(view={self._view.id}, index={self._index})"
