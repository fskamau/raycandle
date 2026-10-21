import asyncio
import websockets
import pandas as pd
from threading import Thread, Event
from typing import Optional, Callable, Tuple
from datetime import datetime, timezone
from enum import IntEnum, unique, Enum, auto


__all__=[
    "UpdateReason",
    "Symbols",
    "Periods",
    "BaseChart",
    "ChartException"
    ]


class ChartException(Exception):
    pass


@unique
class UpdateReason(IntEnum):
    NULL = 0
    TICK = auto()
    NEW_CANDLE = auto()

    def __str__(self):
        return self.name.lower().replace('_', '.')

class Symbols(IntEnum):
    XAUUSD = 73646
    USOIL = 23144
    USDJPY = 8579
    EURUSD = 8577

@unique
class Periods(Enum):
    s1   = (1, 1, "s")
    s10  = (10, 10, "s")
    s30  = (30, 30, "s")
    m1   = (60, 1, "m")
    m5   = (300, 5, "m")
    m10  = (600, 10, "m")
    m15  = (900, 15, "m")
    m30  = (1800, 30, "m")
    h1   = (3600, 1, "h")
    h4   = (14400, 4, "h")
    d1   = (86400, 1, "d")

    def __init__(self, seconds: int, period_value: int, period_unit: str):
        self._seconds = seconds
        self._period_value = period_value
        self._period_unit = period_unit

    @property
    def seconds(self) -> int:
        return self._seconds

    @property
    def period_value(self) -> int:
        return self._period_value

    @property
    def period_unit(self) -> str:
        return self._period_unit

    def __repr__(self) -> str:
        return f'{self._period_value}{self._period_unit}'

    def __str__(self) -> str:
        return self.__repr__()


class BaseChart:
    def __init__(self, symbol:Symbols, period:Periods):
        self.symbol = symbol
        self.period = period
        self.df: pd.DataFrame = pd.DataFrame()
        self.callbacks: list[Callable[[pd.DataFrame], None]] = []
        self.update_reason = UpdateReason.NULL

        self._ws_client = None
        self._thread: Optional[Thread] = None
        self._start_event = Event()
        self._is_done: Optional[asyncio.Future] = None
        self._loop: Optional[asyncio.AbstractEventLoop] = None

        self._curr_ts = 0
        self._curr_o = self._curr_h = self._curr_l = self._curr_c = 0.0

    def add_callback(self, func: Callable[[pd.DataFrame], None]) -> None:
        self.callbacks.append(func)

    def fetch_historical_data(self) -> pd.DataFrame:
        raise NotImplementedError("Implement HTTP Fetch logic")

    async def get_ws_connection(self)->websockets.asyncio.client.ClientConnection:
        """
        open the ws connection and make necessary opening messages if any
        """
        raise NotImplementedError("Implement WS connection")

    async def parse_ws_message(self, ws, msg: str) -> Optional[Tuple[int, float]]:
        raise NotImplementedError("Implement WS message parsing")

    @property
    def running(self) -> bool:
        return self._start_event.is_set() and (self._is_done and not self._is_done.done())

    def start(self) -> None:
        self._thread = Thread(target=self._run_loop, daemon=True)
        self._thread.start()
        self._start_event.wait()

    def fetch(self, stream: bool = False) -> pd.DataFrame:
        if stream:
            self.start()
            return self.df
        return self.fetch_historical_data()

    def close(self) -> None:
        self.callbacks.clear()
        if self._loop and self.running:
            self._loop.call_soon_threadsafe(self._is_done.set_result, None)

    def _run_loop(self):
        asyncio.run(self._async_start())

    def get_df_csv_name(self) -> str:
        return f"data/{self.symbol.name}-{self.period}.csv"

    async def _async_start(self):
        self._loop = asyncio.get_running_loop()
        self._is_done = self._loop.create_future()

        self.df = await self._loop.run_in_executor(None, self.fetch_historical_data)

        if not self.df.empty:
            self._curr_ts = self.df.index[-1]
            self._curr_o = self.df.iat[-1, 0]
            self._curr_h = self.df.iat[-1, 1]
            self._curr_l = self.df.iat[-1, 2]
            self._curr_c = self.df.iat[-1, 3]

        self._ws_client = await self.get_ws_connection()

        self._start_event.set()
        listen_task = self._loop.create_task(self._listen_ws())

        await self._is_done
        listen_task.cancel()
        if self._ws_client:
            await self._ws_client.close()

    async def _listen_ws(self):
        try:
            async for msg in self._ws_client:
                tick = await self.parse_ws_message(self._ws_client, msg)
                if not tick:
                    continue

                epoch, price = tick
                if epoch < self._curr_ts:
                    raise ChartException(f"tick timestamp {epoch} is behind the last known candle {self._curr_ts}")
                last_anchor = self._curr_ts - (self._curr_ts % self.period.seconds)
                current_anchor = epoch - (epoch % self.period.seconds)

                if current_anchor > last_anchor:
                    self._curr_ts = epoch
                    self._curr_o = self._curr_h = self._curr_l = self._curr_c = price
                    self.df.loc[epoch] = [price, price, price, price]
                    self.update_reason = UpdateReason.NEW_CANDLE
                else:
                    if price > self._curr_h: self._curr_h = price
                    if price < self._curr_l: self._curr_l = price
                    self._curr_c = price
                    self._curr_ts = epoch

                    self.df = self.df.rename(index={self.df.index[-1]: epoch})
                    self.df.iloc[-1] = [self._curr_o, self._curr_h, self._curr_l, self._curr_c]
                    self.update_reason = UpdateReason.TICK

                for cb in self.callbacks:
                    cb(self.df)
                self.update_reason = UpdateReason.NULL

        except asyncio.CancelledError:
            pass
        except Exception as e:
            print(f"WS Disconnected/Error: {e}")
