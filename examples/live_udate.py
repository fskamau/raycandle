import pandas as pd
from random import random
import itertools
import raycandle
import numpy as np
import time
from raycandle import cmnfunc as cfc


df = raycandle.fake_stock_data(30000)


class CustomLive:
    SHOWONLY = 200
    """
    Simple illustration of a live update,
    we only show the first 200 items from the data and then update with the rest as time passes
    """

    def __init__(self, df: pd.DataFrame):
        self.fig = fig = raycandle.Figure(
            """
            ab
            cd
            """,
        )

        df_copy = self.prepare_df(df)
        df = df_copy.iloc[: CustomLive.SHOWONLY]
        collector = raycandle.Collector(
            fig,
            fig.ax[0].plot(raycandle.Candle(df[cfc.COLUMNS])),
            [
                fig.ax[0].plot(x, label_from_data=True)
                for x in cfc.bollinger_bands(df["c"])
            ],
            fig.ax[1].plot(df["rsi5"], label_from_data=True),
            fig.ax[1].plot(df["rsi14"], label_from_data=True),
            [fig.ax[2].plot(df[x], label_from_data=True) for x in ["K", "D"]],
            [
                fig.ax[3].plot(df[x], label_from_data=True)
                for x in ["+dmi", "-dmi", "adx"]
            ],
        )
        fig.show_legend()
        fig.set_title(
            "simple Live data update simulation with raycandle.Collector and fig.show(block=False)"
        )
        fig.show_cursors()
        fig.show(False)  # no blocking

        for index in range(CustomLive.SHOWONLY + 1, len(df_copy)):
            if fig.is_window_closed:
                break
            new_df = df_copy.iloc[index - CustomLive.SHOWONLY + 1 : index + 1]
            new_df = CustomLive.prepare_df(new_df)
            collector.update(new_df)
            time.sleep(0.5)

    @staticmethod
    def prepare_df(df: pd.DataFrame) -> pd.DataFrame:
        df = df[cfc.COLUMNS].copy()
        df["ub"], df["mb"], df["lb"] = cfc.bollinger_bands(df["c"])
        df["sma10"] = cfc.sma(df["c"], 10)
        df["sma20"] = cfc.sma(df["c"], 20)

        df["rsi5"] = cfc.rsi(df["c"], 5)
        df["rsi14"] = cfc.rsi(df["c"], 14)

        df["K"], df["D"] = cfc.stoch(*cfc.df2Stuple(df, "hlc"))
        df["+dmi"], df["-dmi"], df["adx"] = cfc.adx(*cfc.df2Stuple(df, "hlc"))
        return df


def main(_=None):
    print(f"file {__name__} running example 1 only")
    CustomLive(df)


if __name__ == "__main__":
    main()
