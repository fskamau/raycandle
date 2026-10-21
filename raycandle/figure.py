"""Simple multi-view plotting on top of raylib + an ABI-stable CFFI surface."""
from __future__ import annotations

import platform
import threading
from contextlib import contextmanager
from pathlib import Path

import cffi

from .bases import RApi, window_not_closed
from .defines import LegendPosition
from .view import View
from .collector import Collector

_PACKAGE=Path(__file__).resolve().parent



class Figure:
    def __init__(self, rows=1, cols=1, *, xdata_format="%Y/%m/%d %H:%M:%S"):
        if not isinstance(rows,int) or not isinstance(cols,int) or rows<1 or cols<1:
            raise ValueError("rows and cols must be positive integers")
        self.rows,self.cols=rows,cols
        ffi=cffi.FFI()
        header = _PACKAGE / "raycandle.h"
        if not header.is_file():
            raise RuntimeError(
                f"Missing generated CFFI header: {header}. "
                "Build the native extension first (see README.md)."
            )
        ffi.cdef(header.read_text(encoding="utf8"))
        if platform.system()=="Windows":
            library="raycandle.dll"
        elif platform.system()=="Darwin":
            library="libraycandle.dylib"
        else:
            library="libraycandle.so"
        target=_PACKAGE/library
        if not target.is_file():
            raise RuntimeError(f"Missing native library: {target}. Build with 'cmake --build build' (see README.md).")
        lib=ffi.dlopen(str(target))
        self._api=RApi(lib,ffi,self)
        self._api.fig=lib.figure_create(rows,cols)
        lib.figure_set_xformat(self._api.fig,self._api.cstr(xdata_format))
        font=_PACKAGE/"assets"/"iosevka-regular.ttf"
        if font.is_file():
            lib.figure_set_font_path(self._api.fig,self._api.cstr(str(font)))
        self.views=[View(self,0)]
        self._closed_event=threading.Event()
        self._thread=None
        self._show_started=False
        self._window_title=None

    @property
    def active_view(self):
        return int(self._api.lib.figure_get_active_view(self._api.fig))

    @property
    def ax(self):
        return self.views[self.active_view].ax

    @window_not_closed
    def add_view(self, data=None, title=None, *, index=None, timeframe=None):
        """Add/select independent view. Optional data/index only initializes its x-axis."""
        if self._show_started:
            raise RuntimeError("Add views before show()")
        view_id=int(self._api.lib.figure_add_view(self._api.fig))
        view=View(self,view_id)
        self.views.append(view)
        if data is not None:
            index=data.index if hasattr(data,"index") else data
        if index is not None:
            view.initialize(index,timeframe=timeframe,title=title)
        return view

    @window_not_closed
    def init_view(self, data, title=None, timeframe=None):
        """Compatibility helper: initialize default view index without drawing."""
        if self.views[0].length:
            raise ValueError("Base view already initialized")
        self.views[0].initialize(data.index if hasattr(data,"index") else data,
                                  title=title,timeframe=timeframe)
        return self.views[0]

    @window_not_closed
    def set_title(self, value):
        """Set the window title used by show(), not a data view label."""
        self._window_title=str(value)
        return self

    @window_not_closed
    def select_view(self, view_id):
        return self.views[view_id].select()

    @window_not_closed
    def set_xdata(self, index, timeframe=None, start=0):
        if timeframe is not None:
            raise ValueError("timeframe is immutable after view initialization")
        return self.views[self.active_view].set_xdata(index,start=start)

    @window_not_closed
    def show_legend(self, position=LegendPosition.BOTTOM_LEFT):
        for axes in self.ax:
            axes.show_legend(position)
        return self

    @window_not_closed
    def show_cursors(self):
        self._api.lib.figure_show_cursors(self._api.fig)
        return self

    @window_not_closed
    def invalidate(self):
        self._api.lib.figure_invalidate(self._api.fig)
        return self

    def _show(self,title,width,height,fps):
        try:
            self._api.lib.figure_show(self._api.fig,self._api.cstr(title),width,height,fps)
        finally:
            self._closed_event.set()

    update=invalidate

    @contextmanager
    def edit(self):
        """Lock drawing while writing directly to artist.data / artist.ydata.

        Do not call set_data/update_tick/select_view inside this context;
        those APIs acquire the same non-reentrant C lock themselves.
        """
        if self.closed: raise RuntimeError("Figure is closed")
        self._api.lib.figure_lock(self._api.fig)
        try:
            yield self
        finally:
            self._api.lib.figure_unlock(self._api.fig)
            self.invalidate()

    @window_not_closed
    def show(self,title=None,width=1280,height=720,fps=60,block=True):
        if self._show_started: raise RuntimeError("show() may be called only once")
        if min(width,height,fps)<=0: raise ValueError("width, height and fps must be positive")
        if not block and platform.system()=="Darwin":
            raise RuntimeError("Raylib requires the main thread on macOS; use show(block=True)")
        self._show_started=True
        if title is None: title=self._window_title
        if block:
            try: self._show(title,width,height,fps)
            finally: self._api.closed=True
        else:
            self._thread=threading.Thread(target=self._show,args=(title,width,height,fps),daemon=True)
            self._thread.start()
            self._api.lib.figure_wait_for_InitWindow(self._api.fig)
        return self

    def wait_closed(self):
        if not self._show_started:
            raise RuntimeError("show() has not been called")
        self._closed_event.wait()
        if self._thread:
            self._thread.join()
        self._api.closed=True

    @property
    def closed(self):
        return self._api.closed or self._closed_event.is_set()

    def close(self):
        """Release C buffers only after the window loop has exited."""
        if self._show_started and not self._closed_event.is_set():
            raise RuntimeError("Close the raylib window before releasing figure memory")
        if not self._api.closed:
            self._api.closed=True
        if self._api.fig != self._api.ffi.NULL:
            self._api.lib.figure_destroy(self._api.fig)
            self._api.fig=self._api.ffi.NULL
