"""Headless regressions: timestamp invariants and CSV market-data validation."""
import tempfile
import unittest
from pathlib import Path

import numpy as np
import pandas as pd

from raycandle.market_data import read_ohlc_csv
from raycandle.timebase import _epochs, _timeframe, _validate_patch


class TimebaseTests(unittest.TestCase):
    def test_timezone_aware_datetime_preserves_epoch_seconds(self):
        dates = pd.date_range("2025-04-01", periods=3, freq="min", tz="Africa/Nairobi")
        actual = _epochs(dates)
        np.testing.assert_allclose(np.diff(actual), [60, 60])
        self.assertGreater(actual[0], 1_700_000_000)
        self.assertEqual(actual.dtype, np.float64)

    def test_rejects_duplicate_reversed_and_nonfinite_times(self):
        for index in ([1, 1], [3, 2], [1, np.nan], [1]):
            with self.subTest(index=index), self.assertRaises(ValueError):
                _epochs(index)

    def test_timeframe_inference_and_invalid_explicit_values(self):
        self.assertEqual(_timeframe(np.array([5., 65., 125.]), None), 60)
        self.assertEqual(_timeframe(np.array([5., 65., 125.]), 300), 300)
        for value in (True, 0, -1, 1.5, float("nan"), float("inf"), "60"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                _timeframe(np.array([1., 2., 3.]), value)

    def test_partial_timestamp_update_checks_neighbors(self):
        old = np.array([10., 20., 30., 40., 50.])
        _validate_patch(old, 2, _epochs([25, 35]))
        for start, values in [(2, [20, 35]), (2, [29, 50]), (-1, [11]), (5, [60, 70])]:
            with self.subTest(start=start, values=values), self.assertRaises(ValueError):
                _validate_patch(old, start, _epochs(values, min_len=1))


class CsvTests(unittest.TestCase):
    def read_text(self, rows):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bars.csv"
            path.write_text("timestamp,open,high,low,close\n" + rows, encoding="utf-8")
            return read_ohlc_csv(path)

    def test_reads_valid_bars_into_chart_columns(self):
        frame = self.read_text(
            "2026-01-01T00:00:00Z,10,12,9,11\n"
            "2026-01-01T00:01:00Z,11,13,10,12\n"
        )
        self.assertEqual(list(frame.columns), ["o", "h", "l", "c"])
        self.assertEqual(frame.index.tz.zone if hasattr(frame.index.tz, "zone") else str(frame.index.tz), "UTC")
        self.assertEqual(frame.c.iloc[-1], 12)

    def test_rejects_bad_prices(self):
        good = "2026-01-01T00:00:00Z,10,12,9,11\n"
        for bad in (
            "2026-01-01T00:01:00Z,11,10,9,12\n",  # close > high
            "2026-01-01T00:01:00Z,11,12,12,11\n", # low > open
            "2026-01-01T00:01:00Z,11,nan,9,12\n",
        ):
            with self.subTest(bad=bad), self.assertRaises(ValueError):
                self.read_text(good + bad)

    def test_rejects_unsorted_and_duplicate_times(self):
        good = "2026-01-01T00:01:00Z,10,12,9,11\n"
        for other in (
            "2026-01-01T00:01:00Z,11,13,10,12\n",
            "2026-01-01T00:00:00Z,11,13,10,12\n",
        ):
            with self.subTest(other=other), self.assertRaises(ValueError):
                self.read_text(good + other)

    def test_example_dataset_is_valid(self):
        file = Path(__file__).resolve().parents[1] / "examples/data/sample_ohlc.csv"
        frame = read_ohlc_csv(file)
        self.assertEqual(len(frame), 72)


if __name__ == "__main__":
    unittest.main()
