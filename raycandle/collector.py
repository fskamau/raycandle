"""Batch updates of aligned native chart data buffers."""

import pandas as pd
from .artists import Candle
from .timebase import _epochs, _validate_patch


class Collector:
    """Update existing artist buffers using only the changed tail of a DataFrame."""
    def __init__(self, figure, *groups):
        self.figure=figure
        self.artists=[]
        def visit(items):
            for obj in items:
                if isinstance(obj,(list,tuple)):
                    visit(obj)
                elif hasattr(obj,"__artist__"):
                    self.artists.append(obj)
                else:
                    raise TypeError(f"Not an artist: {type(obj)}")
        visit(groups)

    def __getitem__(self,index):
        return self.artists[index]

    def update_tail(self, df, timeframe=None):
        """Only copy df's final N values into each matching artist C buffer."""
        if not isinstance(df,pd.DataFrame):
            raise TypeError("Expected a DataFrame")
        if not self.artists: return self
        if df.empty:
            raise ValueError("Tail update requires at least one row")
        view_ids={artist._view_id for artist in self.artists}
        if len(view_ids)!=1:
            raise ValueError("A Collector must contain artists from one view")
        view_id=next(iter(view_ids))
        capacity = None
        for artist in self.artists:
            n = len(artist.ydata)//4 if isinstance(artist, Candle) else len(artist.ydata)
            if len(df)>n:
                raise ValueError("Update is longer than artist capacity")
            names = artist.__data_names__
            required = names if isinstance(names,tuple) else (names,)
            missing = [name for name in required if name not in df.columns]
            if missing:
                raise KeyError(f"Missing artist columns: {missing!r}")
            if capacity is None:
                capacity = n
            elif capacity != n:
                raise ValueError("Collector artists must have equal capacity")
        start = capacity - len(df)
        xs = _epochs(df.index, min_len=1)
        _validate_patch(self.figure.views[view_id].epochs, start, xs)
        for artist in self.artists:
            n=len(artist.ydata)//4 if isinstance(artist,Candle) else len(artist.ydata)
            names=artist.__data_names__
            if isinstance(names,tuple):
                artist.set_data(df.loc[:,list(names)],start=start)
            else:
                artist.set_data(df[names].to_numpy(copy=False),start=start)
        self.figure.views[view_id].set_xdata(df.index,start=start)
        return self

    update=update_tail

    def shift_lines(self, count=1):
        """After a new candle, shift indicator lines and then update their tail."""
        from .artists import Line
        for artist in self.artists:
            if isinstance(artist,Line) and artist.is_splined:
                artist.shift(count)
        return self
