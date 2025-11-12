import argparse
import pandas as pd
import raycandle
import numpy as np


df = raycandle.fake_stock_data(10_000)
random_data = df["c"]


def plotting_lines():
    """Plotting simple lines."""
    fig = raycandle.Figure(window_title="Plotting Lines")
    fig.ax[0].plot(random_data)
    fig.set_title("Without specifying visible_data, all data will be drawn")
    fig.show()


def multi_axes_lines():
    """Multiple lines on multiple axes."""
    fig = raycandle.Figure("12\nvb")
    fig.ax[0].plot(random_data, label="First line")
    fig.ax[0].plot(random_data + random_data.median() * 0.01, label="Second line")
    fig.ax[1].plot(
        random_data * random_data,
        label="Custom color line",
        color=[255, 100, 100, 255],
    )
    fig.show()


def legends_positions():
    """Demonstrate legend positions."""
    fig = raycandle.Figure("12\nvb\n55")
    lp = [str(x) for x in raycandle.LegendPosition]
    for x in range(len(fig.axes) - 1):
        fig.ax[x].plot(random_data, label=f"Example {x}")
        fig.ax[x].set_title(lp[x])
        fig.ax[x].show_legend(x + 1)
    fig.show()


def candlestick_chart():
    """Basic candlestick chart."""
    fig = raycandle.Figure("ab\ncd")
    fig.ax[0].plot(raycandle.Candle(df))
    fig.set_title("Candlesticks")
    fig.show_cursors()
    fig.show()


def indicators_demo():
    """Demonstrate basic technical indicators."""
    fig = raycandle.Figure("ab\ncd\nef")
    fig.ax[0].plot(raycandle.Candle(df))
    for x in raycandle.bollinger_bands(df["c"]):
        fig.ax[0].plot(x, label="Bollinger Bands")

    fig.ax[1].plot(raycandle.sma(df["c"], 10), label="SMA(10)")
    fig.ax[1].plot(raycandle.sma(df["c"], 20), label="SMA(20)")

    for x in raycandle.macd(df["c"]):
        fig.ax[3].plot(x, label="MACD")

    fig.show_legend()
    fig.set_title("Technical Indicators")
    fig.show_cursors()
    fig.show()



    
