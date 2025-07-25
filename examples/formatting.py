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
        fig.set_title("default formatter(Check the tooltip)")
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
        fig.set_xformatter(raycandle.FormatterType.NULL)
        fig.show_legend()
        fig.set_title("NULL xformatter(Check the tooltip)")
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
        fig.set_xformatter(raycandle.FormatterType.LINEAR, "%.3f")
        fig.show_legend()
        fig.set_title("Linear xformatter(Check the tooltip)")
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
