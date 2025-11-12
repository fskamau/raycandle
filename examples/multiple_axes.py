# a figure with multple axes
import raycandle


def one_axes(name):
    print(f"running example {name!r}")
    fig = raycandle.Figure(window_title="1 axes only")
    fig.set_title("this figure has only 1 axes")
    fig.show()


def horizontal_axes(name):
    print(f"running example {name!r}")
    fig = raycandle.Figure(
        window_title="Two axes",
        fig_skel="""
        a
        b
        """,
    )
    fig.set_title("this figure has 2 horizontal axes")
    fig.show()


def vertival_axes(name):
    print(f"running example {name!r}")
    fig = raycandle.Figure(
        window_title="Two axes",
        fig_skel="""
        ab
        """,
    )
    fig.set_title("this figure has 2 vertival axes")
    fig.show()


def equal_axes(name):
    print(f"running example {name!r}")
    fig = raycandle.Figure(
        window_title="4 axes",
        fig_skel="""
        ab
        cd
        """,
    )
    fig.set_title("this figure  has 4 equal axes")
    fig.show()


def unequal_axes(name):
    print(f"running example {name!r}")
    fig = raycandle.Figure(
        window_title="4 axes",
        fig_skel="""
        aab
        cdd
        """,
    )
    fig.set_title("this figure has 4 unequal axes. Axes[0] and axes[3] are bigger")
    fig.show()


def odd_axes(name):
    print(f"running example {name!r}")
    fig = raycandle.Figure(
        window_title="5 axes",
        fig_skel="""
        ab
        cd
        ee
        """,
    )
    fig.set_title(
        "this figure has 5 axes. Since axes are not even, they will not be equal"
    )
    fig.show()


import inspect
import sys
from multiprocessing import Process

if __name__ == "__main__":
    current_module = sys.modules[__name__]
    functions = inspect.getmembers(current_module, inspect.isfunction)
    for name, ex in functions:
        p = Process(target=ex, args=(name,))
        p.start()
        p.join()
