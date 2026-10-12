import threading
import warnings
from typing import Any, NoReturn, Type
import os
import cffi
import numpy as np
import pandas as pd
import signal
from .axes import Axes
from .bases import Artist, RApi, window_not_closed
from .axes import Axes
from .defines import *
from .exceptions import *
from pathlib import Path
from threading import Event


FPATH = Path(os.path.dirname(__file__))
LIB_NAME = FPATH / "libraycandle.so"
CDEF_NAME = FPATH / "raycandle.h"
ASSET_PATH=FPATH/"assets"


class Collector:
    def __init__(self, fig: "Figure", *args: ["Artist"]):
        self._artists, self.__data_names__ = [], set()
        if not isinstance(fig, Figure):
            raise TypeError(f"expected a Figure instance got {type(fig)}")
        self._fig = fig
        self._set_dict([args])

    def __getitem__(self, index):
        return self._artists[index]

    def _set_dict(self, a):
        if issubclass(type(a), Artist):
            if not hasattr(a, "__data_names__"):
                raise NotImplementedError(f"artist {a} has no attribute '__data_names__'")
            self._artists.append(a)
        elif isinstance(a, (tuple, list)):
            tuple(map(self._set_dict, a))
        else:
            raise NotImplementedError(f"{type(a)} is not an Artist")

    def update(self, df: pd.DataFrame, timeframe:int):
        l=self._fig._api.total_len
        if len(df) > l:
            raise RuntimeError(f"length mismatch; prev {l} new {len(df)}")
        start=l-len(df)
        self._fig.set_xdata(df.index, timeframe, start=start)
        if diff := self.__data_names__.difference(df.columns):
            raise ValueError(f"missing column names {diff}")
        tuple(map(lambda x: x.set_data(df[x.__data_names__],start=start), self._artists))
        self._fig._api.fig.redraw_modebar+=1  #trigger just the redraw
        

class Figure:
    def __init__(self, rows: int = 1, cols: int = 1, xdata_format="[%d-%b %a %H:%M:%S]"):
        self._api: RApi
        self.xdata_format = xdata_format
        if rows < 1 or cols < 1:
            raise Exception
        self._wait_closed=Event()
        self._load_lib()
        self._pxdata = self._api.null
        self._api.fig = self._api.lib.figure_create(rows, cols)
        theme = self._api.fig.theme
        ic,fp=self._api.cstr(str(ASSET_PATH/"icon.png")),self._api.cstr(str(ASSET_PATH/"font.ttf"))
        
        theme.icon_path =ic
        theme.font_path = fp
        self._api.lib.figure_set_theme(self._api.fig, theme)
        self._api.this = self        

        self.ax: list[Axes]
        self._init()

    def set_title(self, title: str):
        self._api.lib.figure_set_title(self._api.fig, title.encode())

    def _load_lib(self) -> None:
        ffi = cffi.FFI()
        lib = ffi.dlopen(os.path.join(FPATH, LIB_NAME))
        ffi.cdef(open(os.path.join(FPATH, CDEF_NAME)).read())
        self._api = RApi(lib, ffi.NULL, ffi, self)


    def _init(self) -> None:
        self.ax = [Axes(i, self._api) for i in range(self._api.fig.axes_count)]

    @window_not_closed
    def _show(self, window_title: Optional[str], width: int, height: int, fps: int) -> None:
        self._api.lib.figure_show(self._api.fig, self._api.cstr(window_title), width, height, fps)
        self._api.closed = True
        self._wait_closed.set()

    def _wait_init(self) -> None:
        """
        return when figure is allocated in memory to prevent refrencing to NULL when
        calling from another thread
        """
        self._api.lib.figure_wait_for_InitWindow(self._api.fig)

    @window_not_closed
    def show(self, window_title: str = None, width: int = 1280, height: int = 720, fps: int = 20, block: bool = True) -> None:
        """
        starts a game loop and draws artists on the screen.
        block: whether to use a separate thead to run the game loop.
          if False, this function will return immediately otherwise it will block until the window is closed
        """
        if not block:
            threading.Thread(target=self._show, args=(window_title, width, height, fps)).start()
            self._wait_init()
            return
        self._show(window_title, width, height, fps)


    @window_not_closed
    def show_cursors(self) -> None:
        self._api.lib.figure_show_cursors(self._api.fig)

    @window_not_closed
    def update(self):
        """
        update the figure by recomputing everything
        """
        self._api.fig.force_update = True

    def add_view(self,df:pd.DataFrame,title:str):
        bs=self.__create_bar_series__(title,df)
        self._api.lib.figure_add_view(self._api.fig,bs)
        

    @window_not_closed
    def set_title(self, title: str) -> None:
        self._api.lib.figure_set_title(self._api.fig, self._api.cstr(title))

    def init_view(self,df:pd.DataFrame, title:str=None):
        if self._api.base_view_set:
            raise Exception("base view already called; try figure.add_view")
        self._api.lib.figure_set_base_bar_series(self._api.fig,self.__create_bar_series__(df,title))
        self._api.base_view_set=True

    def __create_bar_series__(self,df:pd.DataFrame, title:str=None):
        assert len(df.columns)==4
        df.columns=[x for x in 'ohlc']
        dfd={x:self._api.ffi.cast('float*',df[x].to_numpy(np.float32).ctypes.data) for x in df.columns}
        index=pd.Series(df.index)
        timeframe = np.bincount([x for x in np.diff(index) if not np.isnan(x)]).argmax()
        if len(pd.Series(index).dropna().diff().iloc[1:].drop_duplicates()) != 1:
            warnings.warn(f"index spacing is not equal, the most occurrent spacing ({timeframe}) will be applied", RuntimeWarning)    
            
        return self._api.lib.bar_series_create(
            timeframe,
            len(df),
            dfd,
            self._api.ffi.cast('double*',index.to_numpy(dtype=np.float64).ctypes.data),
            self._api.arena,
            self._api.cstr(title))
                               
        # BarSeries* bar_series_create(size_t timeframe, size_t len, Candles candles, double* epochs, Arena *arena, const char *title);
        
    @window_not_closed
    def set_xdata(self, xdata: pd.Index, timeframe: int, start: int=0) -> None:
        """
        sets the shared x-axis data with a range
        """
        n=self._api.total_len
        if(n==0):raise Exception("draw some artists first")
        if len(xdata)> n: raise Exception("source len cannot exceed dest len")
        xdata=pd.Series(xdata,dtype=np.float64).to_numpy()
        pxdata=self._api.ffi.cast('double*',xdata.ctypes.data)
        self._api.lib.figure_update_xdata(self._api.fig, pxdata, timeframe,start,len(xdata))

    @window_not_closed
    def show_legend(self, lp: LegendPosition = LegendPosition.BOTTOM_LEFT) -> None:
        self._api.lib.figure_show_legends(self._api.fig, lp)

    def wait_closed(self):
        self._wait_closed.wait()
        
    @property
    def closed(self)->bool:
        return self._api.closed
  
