"""Ownership and common primitives for FFI-backed artists."""
from __future__ import annotations

from functools import wraps


def window_not_closed(func):
    @wraps(func)
    def wrapper(self, *args, **kwargs):
        if self._api.closed:
            raise RuntimeError("Figure has been closed")
        return func(self, *args, **kwargs)
    return wrapper


class RApi:
    def __init__(self, lib, ffi, figure):
        self.lib = lib
        self.ffi = ffi
        self.figure = figure
        self.fig = ffi.NULL
        self.closed = False

    def cstr(self, value):
        """Only use within a call: C copies strings it needs to retain."""
        return self.ffi.NULL if value is None else self.ffi.new("char[]", str(value).encode())


class Artist:
    _api: RApi

    @property
    def ptr(self):
        """Raw C float* pointer. It becomes invalid after Figure.close()."""
        return self._api.lib.artist_get_data_ptr(self.__artist__)

    @window_not_closed
    def invalidate(self):
        """Call after directly modifying the NumPy/pandas zero-copy view."""
        self._api.lib.artist_invalidate(self.__artist__)
        return self

    @window_not_closed
    def ylim_turnoff(self):
        self._api.lib.artist_set_limitable(self.__artist__, 0)
        return self

    @window_not_closed
    def fade(self, alpha):
        if not (0.0 <= alpha <= 1.0):
            raise ValueError("alpha must be between 0 and 1")
        self._api.lib.artist_set_alpha(self.__artist__, round(alpha*255))
        return self

    def __repr__(self):
        return f"{type(self).__name__}(label={self.label!r})"


class Color:
    def __init__(self, r, g, b, a=255):
        self.r,self.g,self.b,self.a = [max(0,min(255,int(v))) for v in (r,g,b,a)]

    @classmethod
    def fromhex(cls, value):
        if value <= 0xFFFFFF:
            value=(value << 8) | 255
        return cls((value >> 24)&255,(value >> 16)&255,(value >> 8)&255,value&255)

    def __iter__(self):
        return iter((self.r,self.g,self.b,self.a))

    def __repr__(self):
        return f"Color({self.r}, {self.g}, {self.b}, {self.a})"
