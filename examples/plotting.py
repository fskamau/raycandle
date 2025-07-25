import pandas as pd
from random import random
import itertools
import raycandle
import numpy as np

df = raycandle.fake_stock_data(10000)
random_data = df["c"]


def main(example_id=None):
    l = itertools.count()
    lnext = lambda: next(l)

    if example_id == lnext():
        fig = raycandle.Figure(window_title="plotting lines")
        fig.set_title(
            "in raycandle.Figure(), without specifying visible_data, all data will be drawn"
            ". without specifying update_len, data cannot be updated"
        )
        fig.ax[0].plot(random_data)
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            """
            12
            vb
            """,
            window_title="figure with minimal amount of text",
            visible_data=100,
            update_len=10,
        )
        for x in fig.ax:
            x.plot(random_data, label=str(x))
        fig.show()

    elif example_id == lnext():
        visible_data = 500
        fig = raycandle.Figure(visible_data=visible_data)
        fig.set_title(
            f"total_data={len(random_data)}; visible data is {visible_data} with no update_len (try moving around)"
        )
        fig.ax[0].plot(random_data)
        fig.show()

    elif example_id == lnext():
        visible_data = 500
        update_len = 10
        fig = raycandle.Figure(visible_data=visible_data, update_len=update_len)
        fig.set_title(
            f"total_data={len(random_data)}; visible data is {visible_data} with  update_len {update_len} (try moving around)"
        )
        fig.ax[0].plot(random_data)
        fig.show()

    elif example_id == lnext():
        visible_data = 100
        update_len = 10
        fig = raycandle.Figure(
            """
            12
            vb
            """,
            visible_data=visible_data,
            update_len=update_len,
        )
        fig.set_title(f"mutliple lines mutliple axes ")
        fig.ax[0].plot(random_data, label="first line")
        fig.ax[0].plot(random_data + random_data.median() * 0.01, label="second line")
        fig.ax[1].plot(
            random_data * random_data,
            label="a line with its own color",
            color=[255, 100, 100, 255],
        )
        fig.ax[2].set_title(
            "this axes has not artists hence no mouse action will take place"
        )
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            """
            12
            vb
            55
            """,
            visible_data=100,
            update_len=10,
        )
        fig.set_title("legend can be in 4 positions")
        lp = [str(x) for x in raycandle.LegendPosition]
        for x in range(len(fig.axes) - 1):
            fig.ax[x].plot(random_data, label=str(x))
            fig.ax[x].set_title(lp[x])
            fig.ax[x].show_legend(x + 1)
        fig.ax[4].plot(random_data)
        fig.ax[4].set_title("no legend")
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            """
            122
            vbn
            555
            """,
            visible_data=100,
            update_len=10,
        )
        fig.set_title("cursors using fig.show_cursors")
        for x in range(4):
            fig.ax[x].plot(random_data, label=str(x))
            fig.ax[x].show_legend(x + 1)
        fig.show_cursors()
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            """
            ab
            cd
            """,
            visible_data=100,
            update_len=10,
        )
        fig.ax[0].plot(raycandle.Candle(df))
        fig.show_legend()
        fig.set_title("candlesticks")
        fig.show_cursors()
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            """
            ab
            cd
            ef
            """,
            visible_data=100,
            update_len=10,
        )
        fig.ax[0].plot(raycandle.Candle(df))
        [
            fig.ax[0].plot(x, label="bollinger_bands")
            for x in raycandle.bollinger_bands(df["c"])
        ]
        fig.ax[1].plot(raycandle.sma(df["c"], 10), label="SMA(10)")
        fig.ax[1].plot(raycandle.sma(df["c"], 20), label="SMA(20)")
        fig.ax[2].plot(raycandle.rsi(df["c"], 14), label="RSI(14)")
        fig.ax[2].plot(raycandle.rsi(df["c"], 21), label="RSI(21)")
        [fig.ax[3].plot(x, label="MACD") for x in raycandle.macd(df["c"])]
        [
            fig.ax[4].plot(x, label="stochastic")
            for x in raycandle.stoch(*raycandle.df2Stuple(df, "hlc"))
        ]
        [
            fig.ax[5].plot(x, label="ADX")
            for x in raycandle.adx(*raycandle.df2Stuple(df, "hlc"))
        ]
        fig.show_legend()
        fig.set_title("simple technical indicators")
        fig.show_cursors()
        fig.show()

    else:
        example_id = 0
        total = lnext()
        while example_id < total:
            print(f"file {__name__} running example {example_id+1} of {total}")
            main(example_id)
            example_id += 1


if __name__ == "__main__":
    main()
