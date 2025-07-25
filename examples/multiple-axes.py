# a figure with multple axes
# comment any
import itertools
import raycandle


def main(example_id=None):
    l = itertools.count()
    lnext = lambda: next(l)

    if example_id == lnext():
        fig = raycandle.Figure(window_title="1 axes only")
        fig.set_title("this figure with only 1 axes")
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            window_title="Two axes",
            fig_skel="""
        a
        b
        """,
        )
        fig.set_title("this figure with has 2 horizontal axes")
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            window_title="Two axes",
            fig_skel="""
        ab
        """,
        )
        fig.set_title("this figure with has 2 vertival axes")
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            window_title="4 axes",
            fig_skel="""
        ab
        cd
        """,
        )
        fig.set_title("this figure with has 4 equal axes")
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            window_title="4 axes",
            fig_skel="""
        aab
        cdd
        """,
        )
        fig.set_title(
            "this figure with has 4 unequal axes. Axes[0] and axes[3] are bigger"
        )
        fig.show()

    elif example_id == lnext():
        fig = raycandle.Figure(
            window_title="5 axes",
            fig_skel="""
        ab
        cd
        ee
        """,
        )
        fig.set_title(
            "this figure with has 5 axes. Since axes are not even, they will not be equal"
        )
        fig.show()

    else:
        example_id = 0
        total = lnext()
        while example_id < total:
            print(f"file {__name__}running example {example_id+1} of {total}")
            main(example_id)
            example_id += 1


if __name__ == "__main__":
    main()
