import pandas as pd
from random import random
import itertools
import raycandle

random_data = pd.Series([random() for x in range(100000)])


def main(example_id=None):
    l = itertools.count()
    lnext = lambda: next(l)

    if example_id == lnext():
        fig = raycandle.Figure(window_title="Using KEY_F")
        fig.set_title("press F to toggle maximized or minimized")
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(window_title="shift keys")
        fig.set_title("press shift to not draw axes")
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            window_title="MOVEMENT",
            visible_data=100,
            update_len=10,
        )
        fig.ax[0].plot(random_data, label="random_data")
        fig.show_legend()
        fig.set_title(
            "[1]press arrow keys to move faster. [2]press h|l to move 1 data point. [3]drag left or right using mouse."
        )
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
