"""Independent plot views backed by a native figure."""

import numpy as np
from .axes import Axes
from .bases import window_not_closed
from .timebase import _epochs, _timeframe, _validate_patch


class View:
    def __init__(self, figure, view_id):
        self._figure=figure
        self._api=figure._api
        self.id=view_id
        self.ax=[Axes(self,i) for i in range(figure.rows*figure.cols)]
        self._len=0

    def select(self):
        f=self._figure
        if f._api.closed: raise RuntimeError("Figure has been closed")
        if f.active_view!=self.id:
            f._api.lib.figure_move_view_on_top(f._api.fig,self.id)
        return self

    @property
    def length(self):
        return self._len

    @property
    def epochs(self):
        """Native float64 x-values; writable only inside `with figure.edit()`."""
        if self._api.closed:
            raise RuntimeError("Figure has been closed")
        if not self._len:
            return np.empty(0,dtype=np.float64)
        f=self._figure._api
        ptr=f.lib.figure_view_get_epochs(f.fig,self.id)
        return np.frombuffer(f.ffi.buffer(f.ffi.cast("double*",ptr),8*self._len), dtype=np.float64)

    def initialize(self, index, *, timeframe=None, title=None):
        if self._api.closed or self._figure._show_started:
            raise RuntimeError("Initialize a view before opening or closing its figure")
        if self._len:
            raise ValueError("View is already initialized")
        xs=_epochs(index)
        if len(xs) < 2: raise ValueError("Need at least two data points")
        tf=_timeframe(xs,timeframe)
        self._figure._api.lib.figure_view_set_data(
            self._figure._api.fig,self.id,len(xs),
            self._figure._api.ffi.cast("double*",xs.ctypes.data),tf,
            self._figure._api.cstr(title))
        self._len=len(xs)
        self.select()
        return self

    def _ensure_data(self, artist):
        if not self._len:
            self.initialize(artist.xdata)
        elif len(artist.xdata)!=self._len and getattr(artist,"is_splined",True):
            raise ValueError(f"View {self.id} expects {self._len} points; got {len(artist.xdata)}")

    @window_not_closed
    def set_xdata(self, index, *, start=0):
        xs=_epochs(index,min_len=1)
        _validate_patch(self.epochs, start, xs)
        self._figure._api.lib.figure_view_update_xdata(
            self._figure._api.fig,self.id,
            self._figure._api.ffi.cast("double*",xs.ctypes.data),start,len(xs))
        return self
