"""Inspect historical OHLC candles and a moving average from a local CSV.

Usage:
    python examples/csv_market_viewer.py examples/data/sample_ohlc.csv

Requires the raylib-based native extension and a desktop display. The input is
read-only; this example does not connect to a broker or place trades.
"""
import argparse

from raycandle import Candle, Figure
from raycandle.market_data import read_ohlc_csv


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", help="UTC ISO timestamp,open,high,low,close CSV")
    args = parser.parse_args()

    bars = read_ohlc_csv(args.csv)
    fig = Figure(rows=2, cols=1)
    fig.ax[0].plot(Candle(bars))
    fig.ax[0].plot(bars.c.rolling(10, min_periods=1).mean(), name="SMA 10")
    fig.ax[1].plot(bars.c.diff().fillna(0), name="Close change")
    fig.set_title("Offline OHLC inspection")
    try:
        fig.show()
    finally:
        fig.close()


if __name__ == "__main__":
    main()
