"""
##
# common functions
##
"""

import time
import warnings
from datetime import datetime
from typing import Any, Callable, Iterable, Optional, Union

import numpy as np
import pandas as pd
from pytz import utc

__all__ = [
    "COLUMNS",
    "FORMAT",
    "adx",
    "aroon",
    "avg_tr",
    "bollinger_bands",
    "cci",
    "convert",
    "cross",
    "demaker",
    "df2Stuple",
    "donchian_channel",
    "ema",
    "epoch2readable",
    "epoch_to_nearest",
    "find_index",
    "get_lim",
    "last_at",
    "load_df",
    "loc2iloc",
    "macd",
    "mdate2readable",
    "meet",
    "mpl2datetime2int",
    "printall",
    "rename",
    "roc",
    "rsi",
    "scale_to_range",
    "sma",
    "sroc",
    "stoch",
    "time_to_nearest",
    "wma",
    "fake_stock_data",
]


COLUMNS = ["o", "h", "l", "c"]
FORMAT = "%d/%H:%M"


def float2str(f: float) -> str:
    return f"{f:.6f}"


def scale_to_range(
    data: Union[pd.Series, pd.DataFrame, tuple[Union[pd.Series, pd.DataFrame]]], scale_range: tuple[float], assume_range: Optional[tuple[float, float]] = None
):
    """
    scales `data` to range(min_val,max_val)=`scale_range`.
    if `assume_range` is None, limits are calculated as [minimum(data), maximum(data)]
    """
    min_val, max_val = scale_range
    if min_val >= max_val:
        raise ValueError("min_val>=max_val")
    # max,min,diff
    A, B, C = None, None, max_val - min_val
    if assume_range is not None:
        A, B = assume_range

    def a(z):
        return (((z - A) / (B - A)) * C) + min_val

    if isinstance(data, pd.Series):
        if assume_range is None:
            A = np.nanmin(data)
            B = np.nanmax(data)
        return a(data)
    if isinstance(data, pd.DataFrame):
        # if data.isna().any().any():
        #     raise ValueError("cannot scale nans")
        if assume_range is None:
            A, B = data.min().min(), data.max().max()
        return data.apply(a)
    if isinstance(data, (tuple, list)):
        return type(data)((map(lambda x: scale_to_range(x, scale_range, assume_range), data)))
    raise TypeError("invalid type ", type(data))


def rename(data: Union[pd.Series, pd.DataFrame]) -> Union[pd.Series, pd.DataFrame]:
    """
    rename `df`'s columns to `COLUMNS`
    """
    intrest = data.columns if isinstance(data, pd.DataFrame) else data.index
    if len(intrest) != len(COLUMNS):
        raise ValueError(f"expected {len(COLUMNS)} columns found {len(intrest)}")
    if len(set(COLUMNS).difference(set(intrest))) == 0:
        return data
    if isinstance(data, pd.DataFrame):
        data.columns = COLUMNS
    else:
        data.index = COLUMNS
    data.index = pd.Series(data.index, name="d")
    return data


def mdate2readable(x):
    return datetime.strftime(mdates.num2date(x), FORMAT)


def sma(ser: Union[pd.Series, pd.DataFrame], window: int, name: Optional[str] = None):
    """
    Simple Moving Average
    """

    return pd.Series(ser.rolling(window=window).mean(), index=ser.index, name=name if name else ser.name)


def ema(ser: Union[pd.Series, pd.DataFrame], window: int, name: Optional[str] = None):
    return pd.Series(ser.ewm(span=window).mean(), index=ser.index, name=name if name else ser.name)


def convert(loc1: int):
    """Makes a datetime.datetime object without a timezone from an epoch timestamp"""
    return datetime.strptime(
        utc.localize(datetime.fromtimestamp(loc1)).strftime(
            "%y-%m-%d %H:%M",
        ),
        "%y-%m-%d %H:%M",
    )


def avg_tr(low: pd.Series, high: pd.Series, close: pd.Series, window: int = 14, name="avg_tr") -> pd.Series:
    """
    Average true range
    """
    hml = high - low
    hmc_shifted = (high - close.shift(1)).abs()
    lmc_shifted = (low - close.shift(1)).abs()
    true_range = pd.DataFrame(
        {
            1: hml,
            2: hmc_shifted,
            3: lmc_shifted,
        }
    ).max(axis=1)
    average_true_range = true_range.ewm(window, adjust=False).mean()
    average_true_range.name = name
    return average_true_range


def stoch(
    high: pd.Series,
    low: pd.Series,
    close: pd.Series,
    k=14,
    d=3,
    m=3,
    names: list[str] = ["k", "d"],
) -> tuple[pd.Series, pd.Series]:
    """
    stochastic indicator
    """
    high = high.rolling(k).max()
    low = low.rolling(k).min()
    num = close - low
    denom = high - low
    k = sma((num / denom) * 100, m)
    D = sma(k, d)
    D.name, k.name = names[0], names[1]
    return D, k


def wma(ser: pd.Series, period: int = 10, name: Optional[str] = None):
    """
    weighted moving average
    """
    index = ser.index
    close = ser.values
    weights = np.arange(1, period + 1)
    weights_sum = np.sum(weights)
    wma = np.zeros(len(ser))
    for i in range(period - 1, len(ser)):
        wma[i] = np.dot(weights, close[(i - period + 1) : (i + 1)]) / weights_sum
    wma = np.where(wma == 0, np.nan, wma)
    return pd.Series(wma, index=index, name=ser.name if name is None else name)


def macd(ser: pd.Series, x=12, y=26, z=9, names: tuple[str, str] = ("m", "s")) -> tuple[pd.Series]:
    """
    Moving Average Convergence Divergence
    """
    ema_12 = ser.ewm(span=x, adjust=False).mean()
    ema_26 = ser.ewm(span=y, adjust=False).mean()
    md = pd.Series(ema_12 - ema_26, name=names[0])
    signal = pd.Series(md.ewm(span=z, adjust=False).mean(), name=names[1])
    return signal, md


def roc(ser: pd.Series, period=14, name: Optional[str] = None):
    """
    Rate Of Change

    NOTE:
    ----
    this function uses price.diff(periods)/price.shift(periods). There are many
    variations of rate of change
    """
    return pd.Series(ser.diff(periods=period) / ser.shift(periods=period), index=ser.index, name=name if name is not None else ser.name)


def sroc(
    ser: pd.Series,
    period: int = 14,
    smoothing_period: int = 9,
    names: tuple[str, str] = ["roc", "sroc"],
    **kwargs,
) -> pd.DataFrame:
    """
    Smoothed Rate of Change

    returns [Rate of Change (ROC), and its smoothed version using Simple Moving Average (SMA)]
    """
    r = roc(ser, period, names[0])
    s = sma(r, smoothing_period, name=names[1])
    r = pd.concat([r, s], axis=1)
    if kwargs:
        return scale_to_range(r, **kwargs)
    return r


def load_df(filename: str, convert_2_readable=False) -> pd.DataFrame:
    """
    args
    ----
    filename: name of the data
    convert_2_readable: format epoch index to readable str
    """
    df = pd.read_csv(filename, index_col=0)
    index = pd.Series(df.index, index=df.index, dtype=np.int32)
    if convert_2_readable:
        index = index.apply(epoch2readable)
    df.index = index
    return df


def meet(df: pd.DataFrame, lag: int = 0) -> np.array:
    """
    returns all starting meeting points of `df` cols values
    ----
    args
    ----
    `df`: the df
    `lag`: how long the meet must last

    returns
    -------
    a numpy array of shape (n,2)
    (the meets points,how long the meets lasted)
    """
    df = df.copy()
    df.reset_index(inplace=True, drop=True)
    previous = False
    crossed, lagcpl = [], []
    lagcp = 0
    for x in df.iterrows():
        if all(x[1].iloc[0] == x[1]):
            if not previous:
                lagcp, previous = 0, True
                crossed.append(x[0])
            else:
                lagcp += 1
        else:
            if previous:
                previous = False
                if lagcp < lag:
                    crossed = crossed[:-1]
                else:
                    lagcpl.append(lagcp)
    if previous:
        crossed = crossed[:-1]
    return np.array([crossed, lagcpl])


def donchian_channel(
    high: pd.Series,
    low: pd.Series = None,
    period: int = 14,
    names: list[str] = ["upper_channel", "middle_channel", "lower_channel"],
    shift: bool = False,
) -> tuple[pd.Series, pd.Series]:
    """
    Donchian Channel

    if `low` is None then low is equated with `high`
    """
    uc = pd.Series(high.rolling(period).max(), name=names[0])
    if type(low) == type(None):
        low = high
    lc = pd.Series(low.rolling(period).min(), name=names[1])
    mc = (lc + uc) / 2
    if shift == True:
        uc = uc.shift(period)
        lc = lc.shift(period)
        mc = mc.shift(period)
    return uc, mc, lc


def cross(ser1: pd.Series, ser2: pd.Series) -> list[Union[int, None]]:
    """
    returns
    -------
    `iloc` crossing point of len <=`count` `iff` `count` is not None else all between `ser1` and `ser2` in ascending order.
    """

    assert len(ser1) == len(ser2)
    for x in [ser1, ser2]:
        if x.isna().any():
            raise Exception("ser contains np.nan")
    x = []
    CC = ser1.iloc[0] > ser2.iloc[0]
    for index in range(1, len(ser1)):
        if CC != (ser1.iloc[index] > ser2.iloc[index]):
            CC = not CC
            x.append(index)
    return x


def rsi(ser: pd.Series, period: int = 14, name: Optional[str] = None):
    """
    Relative Strength Index
    """
    ser = ser.diff()
    up = ser.clip(lower=0)
    down = -1 * ser.clip(upper=0)
    ma_up = sma(up, period)
    ma_down = sma(down, period)
    r = ma_up / ma_down
    r = 100 - (100 / (1 + r))
    if name is not None:
        r.name = name
    return r


def aroon(high: pd.Series, low: pd.Series, period: int = 14, names: list["str"] = ["aroon_up", "aroon_down"]) -> tuple[pd.Series, pd.Series]:
    return (
        pd.Series(100 * ((period - (high.rolling(window=period).apply(lambda x: period - np.argmax(x), raw=True))) / period), name=names[0]),
        pd.Series(100 * ((period - (low.rolling(window=period).apply(lambda x: period - np.argmin(x), raw=True))) / period), name=names[1]),
    )


def bollinger_bands(ser: pd.Series, window: int = 30, names: tuple[str, str, str] = ["ub", "mb", "lb"]) -> tuple[pd.Series, pd.Series, pd.Series]:
    mb = ser.rolling(window=window).mean()
    y = ser.rolling(window=window).std()
    ub = mb + 2 * y
    lb = mb - 2 * y
    for s, name in zip((ub, mb, lb), names):
        s.name = name
    return [ub, mb, lb][: len(names)]


def printall():
    pd.set_option("display.width", None)
    pd.set_option("display.max_rows", None)
    pd.set_option("display.max_columns", None)
    pd.set_option("display.float_format", "{:8.6f}".format)
    np.set_printoptions(precision=4, suppress=True, legacy="1.13", formatter={"str_kind": lambda x: x, "float": "{:8.3f}".format})


def mpl2datetime2int(mpl_time: float):
    """convert a matplotlib date to human readable date and return  minutes in hour+minute"""
    humn_rdble = mdates.num2date(mpl_time)
    return (humn_rdble.hour * 60) + humn_rdble.minute


def epoch2readable(epoch: int, format: str = FORMAT) -> str:
    """
    convert a `epoch` to strftime using `format`
    """
    return datetime.fromtimestamp(epoch).strftime(format)


def time_to_nearest(relation: int, epoch: Optional[int] = None, format: str = FORMAT) -> str:
    """
    return
    ------
    human readable time in `format` to the nearest `relation` seconds. from `epoch`
    if `epoch` is `None`, current epoch will be used
    """
    epoch = epoch_to_nearest(relation, epoch)
    return epoch2readable(epoch, format)


def epoch_to_nearest(relation: int, epoch: Optional[int] = None) -> int:
    """
    returns `epoch` to the nearest `relation` seconds.
    if `epoch` is `None`, current time's epoch will be used
    """
    if epoch is None:
        epoch = int(time.time())
    return int(epoch - (epoch % relation))


def find_index(df: pd.DataFrame, index: float, strict: bool = True) -> Optional[Union[int, tuple[float, float]]]:
    """
    finds the location of `index` in the `df.index` IFF the indexing is in ascending order.
    ---
    args
    ----
    strict:bool whether to find the nearest index to `index` if an exact match lacks.
                It may be also used to cover for bit loss in float comparisons
    returns
    -------
        None if no index was found
        int: location of the `index` if `strict` is True
        tuple[float,float]: tuple[iloc integer location of the match,the match] if `strict` is False
    """

    match = pd.Series(df.index[df.index >= index])
    if not match.is_monotonic_increasing:
        raise NotImplementedError("for now, find_index may only be used for indexes with ascending order")
    if match.empty:
        return None
    if match.iloc[0] != index and strict:
        return None
    index = len(df) - len(match)
    if not strict:
        index = (index, match.iloc[0])
    return index


def get_lim(data: Union[pd.DataFrame, pd.Series, tuple[pd.Series]], ratio: float = 0.05) -> tuple[float, float]:
    """
    gets the data max and min limits.
    return tuple[min,max] limits
    """
    if not isinstance(data, pd.DataFrame):
        if isinstance(data, pd.Series):
            data = [data]
        data = pd.DataFrame(dict(zip((str(x) for x in range(len(data))), data)))
    data = data.dropna().replace(np.Inf, 0)
    a, b = data.min().min(), data.max().max()
    offset = ratio * abs(a - b)
    lims = (a - offset, b + offset)
    if np.isnan(lims).any():
        warnings.warn("Axis limits are Nan")
    return lims


def last_at(data: Union[pd.DataFrame, pd.Series], at: list[Union[int, float]]) -> list[Union[int, list[int]]]:
    """
    returns all integer indexes where data was `at`
    ::

            d          jb
        1           96.396966
        2           95.780903
        3           96.087852
        4           100.000000
        5           100.000000
        6           96.3340909
        >>> print(last_at(data,[100]))
        >>> [5]
    """

    def _last_at(ser: pd.Series, at):
        ser.reset_index(inplace=True, drop=True)
        last = []
        for a in at:
            d = pd.Series(ser[ser == a].index)
            x = d.diff().iloc[1:]
            if x.empty:
                last.append([])
                break
            last.append(d.iloc[pd.Series(x[x != 1.0].index) - 1].to_list() + [d.iloc[-1]])
        return last[0] if len(at) == 1 else last

    return [_last_at(data[col], at) for col in data] if isinstance(data, pd.DataFrame) else _last_at(data, at)


def loc2iloc(loc: Union[Any, Iterable[Any]], data: Union[pd.Series, pd.DataFrame]) -> Union[int, tuple[int]]:
    """
    returns an integer locator from the locator in `df`.
    will raise an IndexError if `loc` is not present.
    """
    if isinstance(data, pd.Series):
        data = pd.concat([data], axis=0)
    index = pd.Series(data.index)

    def _si(l) -> int:
        try:
            return index[index == l].index[0]
        except IndexError as e:
            raise IndexError(f"Index {l} not found; exception: `{e}`")

    if isinstance(loc, (list, tuple)):
        return tuple(map(_si, loc))
    else:
        return _si(loc)


def df2Stuple(df: pd.DataFrame, cols: Iterable[str] = None) -> tuple[pd.Series]:
    """
    dataframe to series tuples.

    eg::

        >>> import pandas as pd
        >>> import numpy as np
        >>> df=pd.DataFrame(dict(zip('abcd',np.arange(0,120,10).reshape((4,3)))))
        >>> df
            a   b   c    d
        0   0  30  60   90
        1  10  40  70  100
        2  20  50  80  110
        >>> df2Stuple(df,['a','c','d'])
        [0     0
        1    10
        2    20
        Name: a, dtype: int64, 0    60
        1    70
        2    80
        Name: c, dtype: int64, 0     90
        1    100
        2    110
        Name: d, dtype: int64]
    """
    if cols is None:
        cols = df.columns
    return [df[col] for col in cols]


def adx(
    high: pd.Series,
    low: pd.Series,
    close: pd.Series,
    window: int = 14,
    names=["+dmi", "-dmi", "adx"],
    use_custom_alpha: bool = False,
    shift_with_window: bool = False,
    low_shift_m_low: bool = False,
) -> tuple[pd.Series, pd.Series, pd.Series]:
    """
    Average Directional Index
    """
    average_true_range = avg_tr(low, high, close, window=window)
    hm_previous_h = high - high.shift(window if shift_with_window else 1)
    previous_l_ml = (low - low.shift(window if shift_with_window else 1)) if low_shift_m_low else (low.shift(window if shift_with_window else 1) - low)
    positive_dx = pd.Series(np.where((hm_previous_h > previous_l_ml) & (hm_previous_h > 0), hm_previous_h, 0), index=low.index)
    negative_dx = pd.Series(np.where((previous_l_ml > hm_previous_h) & (previous_l_ml > 0), previous_l_ml, 0), index=low.index)
    smoothed_positive_dx = positive_dx.ewm(alpha=1 / window, adjust=False).mean()
    smoothed_negative_dx = negative_dx.ewm(alpha=1 / window, adjust=False).mean()
    positive_dmi = (smoothed_positive_dx / average_true_range) * 100
    negative_dmi = (smoothed_negative_dx / average_true_range) * 100
    dx = ((positive_dmi - negative_dmi).abs() / (positive_dmi + negative_dmi)) * 100
    adx = dx.ewm(alpha=1.0 if use_custom_alpha else (1.0 / window), adjust=False).mean()
    for ser, col in zip((positive_dmi, negative_dmi, adx), names):
        ser.name = col
    return [positive_dmi, negative_dmi, adx][: len(names)]


def cci(low: pd.Series, high: pd.Series, close: pd.Series, window: int = 14, name: Optional[str] = None):
    """
    Commodity Channel Index.
    """
    typical_price = (close + high + low) / 3
    c = (typical_price - sma(typical_price, window)) / (0.015 * (typical_price - typical_price.mean()).abs().mean())
    if name:
        c.name = name
    return c


def demaker(high: pd.Series, low: pd.Series, period: int = 14, name: str = None):
    dmax = sma((high - high.shift(1)).apply(lambda x: x if x > 0 else 0), period)
    dmin = sma((low.shift(1) - low).apply(lambda x: x if x > 0 else 0), period)
    return pd.Series(dmax / (dmax + dmin), name=name)


def fake_stock_data(length: int = 600, seconds: int = 600) -> pd.DataFrame:
    interval = f"{seconds}s"
    end_time = pd.Timestamp.now().floor("s")
    start_time = end_time - pd.Timedelta(seconds=(length - 1) * seconds)
    time_index = pd.date_range(start=start_time, periods=length, freq=interval)
    epoch_index = time_index.astype(np.int64) // 10**9

    np.random.seed(int(time.time()))
    prices = [1000]
    trend = 1
    step = 0.2
    volatility = 0.5
    segment_length = np.random.randint(20, 50)
    counter = 0

    for _ in range(1, length):
        if counter >= segment_length:
            trend = np.random.choice([-1, 0, 1])
            segment_length = np.random.randint(20, 50)
            counter = 0
        decay = 1 - (counter / segment_length)
        movement = trend * step * decay + np.random.normal(scale=volatility * decay)
        prices.append(prices[-1] + movement)
        counter += 1

    open_prices = np.array(prices)
    high_spread = np.abs(np.random.normal(loc=0.4, scale=0.15, size=length))
    low_spread = np.abs(np.random.normal(loc=0.4, scale=0.15, size=length))
    high_prices = open_prices + high_spread
    low_prices = open_prices - low_spread
    close_prices = low_prices + (high_prices - low_prices) * np.random.rand(length)

    df = pd.DataFrame({"o": open_prices, "h": high_prices, "l": low_prices, "c": close_prices}, index=epoch_index)

    df.index.name = "d"
    return df
