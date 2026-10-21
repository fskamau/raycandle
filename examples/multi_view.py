"""Self-contained example: two independent time scales and six axes."""
import numpy as np
import pandas as pd
from raycandle import Figure,Candle,Collector

rng=np.random.default_rng(42)
n=200
close=100+np.cumsum(rng.standard_normal(n)).astype(np.float32)
raw=pd.DataFrame({
    "o":close-0.2, "h":close+0.7, "l":close-0.9, "c":close,
},index=pd.date_range("2026-01-01",periods=n,freq="min",tz="UTC"))
raw["sma"]=raw.c.rolling(20,min_periods=1).mean()

fig=Figure(3,2)
a=fig.ax[0].plot(Candle(raw[["o","h","l","c"]]))
line=fig.ax[1].plot(raw.sma,name="SMA",lw=2)

second=raw.iloc[::5].copy()
view=fig.add_view(second,title="Five-minute samples",timeframe=300)
view.ax[0].plot(Candle(second[["o","h","l","c"]]))
view.ax[2].plot(second.sma,name="SMA (5m)")

fig.select_view(0)
fig.show()  # press 0 and 1 to switch the independent views
