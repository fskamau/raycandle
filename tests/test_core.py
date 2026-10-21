"""Native integration tests. Run after build/install; no display server needed."""
import unittest
import os
import sys
from pathlib import Path
import numpy as np
import pandas as pd
import raycandle as rc


_PACKAGE = Path(__file__).resolve().parents[1] / "raycandle"
_NATIVE_BINARY = {"win32": "raycandle.dll", "darwin": "libraycandle.dylib"}.get(sys.platform, "libraycandle.so")
_NATIVE_READY = (_PACKAGE / "raycandle.h").exists() and (_PACKAGE / _NATIVE_BINARY).exists()


@unittest.skipUnless(_NATIVE_READY, "Native raycandle build required; see README build instructions")
class Tests(unittest.TestCase):
    def make_df(self,n=200):
        return pd.DataFrame({name: np.arange(n,dtype="float32") + k
                             for k,name in enumerate(("o","h","l","c"))},
                            index=pd.RangeIndex(1000,1000+n))

    def test_empty_figure(self):
        fig=rc.Figure()
        self.assertEqual(len(fig.ax),1)
        self.assertEqual(fig.views[0].length,0)
        fig.close()

    def test_independent_views(self):
        fig=rc.Figure(3,2)
        df=self.make_df()
        candle=fig.ax[0].plot(df)
        other=fig.add_view()
        second=other.ax[3].plot(pd.Series(np.arange(40,dtype='float32'),name='small'))
        self.assertEqual(fig.active_view,1)
        self.assertEqual(other.length,40)
        self.assertEqual(fig.views[0].length,len(df))
        self.assertIsNot(candle,second)
        fig.select_view(0)
        candle.set_data(df.tail(2),start=198)
        self.assertEqual(candle.data.iloc[-1,3],202)
        fig.close()

    def test_zero_copy(self):
        fig=rc.Figure()
        df=self.make_df(100)
        candle=fig.ax[0].plot(rc.Candle(df))
        line=fig.ax[0].plot(df.c)
        self.assertTrue(np.shares_memory(candle.data.to_numpy(copy=False),candle.ydata))
        self.assertTrue(np.shares_memory(line.data.to_numpy(copy=False),line.ydata))
        with fig.edit():
            candle.data.iloc[0,0]=99
            line.data.iloc[0]=98
        self.assertEqual(candle.ydata[0],99)
        self.assertEqual(line.ydata[0],98)
        fig.close()

    def test_tick_and_line_tail(self):
        fig=rc.Figure()
        candle=fig.ax[0].plot(self.make_df(5))
        line=fig.ax[0].plot(pd.Series([1,2,3,4,5],index=candle.xdata,name='calc'))
        self.assertFalse(candle.update_tick(1004.5,15))
        self.assertTrue(candle.update_tick(1005,16))
        np.testing.assert_allclose(candle.data.iloc[-1].values,16)
        line.shift(1).set_data([10],4)
        np.testing.assert_allclose(line.ydata,[2,3,4,5,10])
        np.testing.assert_allclose(fig.views[0].epochs,[1001,1002,1003,1004.5,1005])
        fig.close()

    def test_rejects_nonmonotonic_partial_epoch_write(self):
        fig = rc.Figure()
        try:
            candle = fig.ax[0].plot(self.make_df(5))
            original = fig.views[0].epochs.copy()
            with self.assertRaises(ValueError):
                fig.views[0].set_xdata([9999], start=1)
            np.testing.assert_array_equal(fig.views[0].epochs, original)
        finally:
            fig.close()

    def test_collector_invalid_columns_are_atomic(self):
        fig = rc.Figure()
        try:
            df = self.make_df(6)
            candle = fig.ax[0].plot(df)
            line = fig.ax[0].plot(pd.Series([1, 2, 3, 4, 5, 6], index=df.index, name="indicator"))
            collector = rc.Collector(fig, candle, line)
            previous = candle.ydata.copy()
            with self.assertRaises(KeyError):
                collector.update_tail(df.tail(1))  # indicator is missing
            np.testing.assert_array_equal(candle.ydata, previous)
        finally:
            fig.close()

    def test_collector_slice(self):
        fig=rc.Figure()
        df=self.make_df(20)
        df['indicator']=np.arange(20,dtype='float32')
        candle=fig.ax[0].plot(df[list('ohlc')])
        line=fig.ax[1-1].plot(df['indicator'])
        collector=rc.Collector(fig,candle,[line])
        delta=df.tail(2).copy()
        delta['indicator']=400
        collector.update_tail(delta)
        self.assertEqual(float(line.data.iloc[-1]),400)
        fig.close()


if __name__=='__main__':
    unittest.main()
