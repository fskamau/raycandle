from .base import *
import re
import struct
import random
import string
import requests
import numpy as np
import pandas as pd
import websockets
import asyncio
from datetime import datetime
__all__=[
    "Centralcharts",
    ]


class CentralChartsException(Exception):
    pass


class CentralCharts(BaseChart):
    Key = "94d690d7869aecc7052af769f28ee883ac3dcf70b4653dd6c1a866dcbd10b485"
    appSessionID = "c526921d0cd7f7cdb5459f0c104ca18b9176d9798eadfd41d1d4a491cb56cd09"
    Token = "a37a8c55"

    HttpUrl = f"https://centralcharts.it-finance.com/expresso/CentralChartsHTML5Public/xlightData?key={Key}"
    WsUrl = "wss://centralcharts.it-finance.com/cappustreamo/websocket/?EIO=4&transport=websocket"

    TickPattern = re.compile(r'\[\["(\d+)","(\d+)","(\d+)",([\d.]+),')

    def __init__(self, symbol: Symbols = Symbols.XAUUSD, period: Periods = Periods.m10):
        super().__init__(symbol,period)

    def fetch_historical_data(self) -> pd.DataFrame:
        body = {
            "periodUnit": self.period.period_unit,
            "parameters": {
                "x": self.period.period_value,
                "dirname": "CentralChartsHTML5Public",
                "profile": "CentralCharts",
                "appSessionID": self.appSessionID,
                "key": self.Key,
                "uid": "public",
                "mode": "X_LIGHT",
                "dividends": True,
                "rollovers": True,
                "instrumentConfig": [
                    {
                        "volume": True,
                        "instrumentID": str(self.symbol.value),
                        "accessType": "RT",
                        "customDayOffset": 0,
                    }
                ],
            },
        }

        headers = {
            "Accept-Language": "en-US,en;q=0.9",
            "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko)",
            "Origin": "https://centralcharts.it-finance.com",
        }

        r = requests.post(self.HttpUrl, headers=headers, json=body)
        if r.status_code != 200:
            raise ValueError(f"HTTP Status {r.status_code}")

        return CentralChartsParser(r.content).parse()

    async def get_ws_connection(self):
        headers = {
            "Origin": "https://centralcharts.it-finance.com",
            "User-Agent": "Mozilla/5.0 (X11; Linux x86_64; rv:144.0) Gecko/20100101 Firefox/144.0",
        }
        ws=await websockets.connect(self.WsUrl, additional_headers=headers)
        req_id = "".join([random.choice(string.ascii_letters + string.digits) for _ in range(8)])
        msgs = [
            f'40/dataStream,{{"key":"{self.Key}","appSessionID":"{self.appSessionID}","profile":"CentralCharts","dirname":"CentralChartsHTML5Public","uid":"public","appName":"itcharts_html5_v3"}}',
            f'42/dataStream,["stream_sub",{{"reqId":"{req_id}","dataSource":"itf","request":[{{"access":"RT","id":"{self.symbol.value}","period":"1T","token":"{self.Token}","lastHistoryRequestTime":null,"topUp":false,"dividends":true,"rollovers":true}}]}}]',
            f'42/dataStream,["preopen_stream_sub",{{"reqId":"{req_id}","dataSource":"itf","request":[{{"access":"RT","id":"{self.symbol.value}","period":"1T","token":"{self.Token}","lastHistoryRequestTime":null,"topUp":false,"dividends":true,"rollovers":true}}]}}]',
        ]
        for m in msgs:
            await ws.send(m)
            await asyncio.sleep(0.1)
        return ws

    async def parse_ws_message(self, ws, msg: str) -> Optional[Tuple[int, float]]:
        if msg == "2":
            await ws.send("3")
            return None

        m = self.TickPattern.search(msg)
        if not m:
            return None

        h, mnt, s, price = m.groups()
        local_dt = datetime.now(timezone.utc).replace(
            hour=int(h), minute=int(mnt), second=int(s), microsecond=0
        ).astimezone()

        return int(local_dt.timestamp()), float(price)



_STRUCT_UINT16 = struct.Struct('>H')
_STRUCT_UINT32 = struct.Struct('>I')
_STRUCT_UINT64 = struct.Struct('>Q')
_STRUCT_INT8 = struct.Struct('>b')
_STRUCT_INT16 = struct.Struct('>h')

class CentralChartsParser:
    def __init__(self, data: bytes):
        self.data = data
        self.len_ = len(data)
        self.offset = 0

    def parse(self) -> pd.DataFrame:
        start_histo = _STRUCT_INT16.unpack_from(self.data, self.offset)[0] + 2
        self.offset = start_histo

        frames = []
        while self.offset < self.len_:
            frame = self._parse_trame()
            if frame: frames.append(frame)

        if not frames:
            return pd.DataFrame()

        frames.sort(key=lambda x: x['id'])

        all_d, all_o, all_h, all_l, all_c = [], [], [], [], []
        for f in frames:
            if f['count'] > 0:
                all_d.append(f['timeStart'])
                all_o.append(f['open'])
                all_h.append(f['high'])
                all_l.append(f['low'])
                all_c.append(f['close'])

        if not all_d:
            return pd.DataFrame()

        return pd.DataFrame({
            'o': np.concatenate(all_o),
            'h': np.concatenate(all_h),
            'l': np.concatenate(all_l),
            'c': np.concatenate(all_c)
        }, index=pd.Series(np.concatenate(all_d), name="d"))

    def _parse_trame(self):
        start_pos = self.offset

        trame_size = _STRUCT_UINT16.unpack_from(self.data, start_pos)[0]
        header_size = _STRUCT_UINT16.unpack_from(self.data, start_pos + 2)[0]
        p_type = _STRUCT_UINT16.unpack_from(self.data, start_pos + 4)[0]
        trame_number = _STRUCT_UINT64.unpack_from(self.data, start_pos + 6)[0]

        if not (1 == p_type or 4 <= p_type <= 7):
            raise CentralChartsException(f"Invalid frame type: {p_type}")

        trame_end = start_pos + trame_size + 2
        self.offset = start_pos + header_size + 4

        nb_el = _STRUCT_UINT16.unpack_from(self.data, self.offset + 4)[0]
        ts_factor = _STRUCT_UINT64.unpack_from(self.data, self.offset + 6)[0]
        exponent_raw = _STRUCT_INT8.unpack_from(self.data, self.offset + 14)[0]

        if nb_el == 0:
            self.offset = trame_end
            return {'id': trame_number, 'count': 0}

        ts_factor_dbl = ts_factor / 1e9
        price_exp = 10.0 ** exponent_raw

        data = self.data
        offset = self.offset + 15

        raw_time, raw_open, raw_high, raw_low, raw_close = [], [], [], [], []

        for _ in range(nb_el):
            res = shift = 0
            while True:
                b = data[offset]; offset += 1
                res |= (b & 0x7F) << shift
                if not (b & 0x80): break
                shift += 7
            raw_time.append((res >> 1) ^ -(res & 1))

            while data[offset] & 0x80: offset += 1
            offset += 1

            res = shift = 0
            while True:
                b = data[offset]; offset += 1
                res |= (b & 0x7F) << shift
                if not (b & 0x80): break
                shift += 7
            raw_open.append((res >> 1) ^ -(res & 1))

            res = shift = 0
            while True:
                b = data[offset]; offset += 1
                res |= (b & 0x7F) << shift
                if not (b & 0x80): break
                shift += 7
            raw_high.append((res >> 1) ^ -(res & 1))

            res = shift = 0
            while True:
                b = data[offset]; offset += 1
                res |= (b & 0x7F) << shift
                if not (b & 0x80): break
                shift += 7
            raw_low.append((res >> 1) ^ -(res & 1))

            res = shift = 0
            while True:
                b = data[offset]; offset += 1
                res |= (b & 0x7F) << shift
                if not (b & 0x80): break
                shift += 7
            raw_close.append((res >> 1) ^ -(res & 1))

            while data[offset] & 0x80: offset += 1
            offset += 1

        self.offset = trame_end

        raw_time = np.array(raw_time, dtype=np.float64)
        raw_open = np.array(raw_open, dtype=np.float64)
        raw_high = np.array(raw_high, dtype=np.float64)
        raw_low = np.array(raw_low, dtype=np.float64)
        raw_close = np.array(raw_close, dtype=np.float64)

        timestamps = np.cumsum(raw_time * ts_factor_dbl).astype(np.int64)

        d_open, d_high, d_low, d_close = raw_open * price_exp, raw_high * price_exp, raw_low * price_exp, raw_close * price_exp

        closes = np.cumsum(d_open + d_close)
        opens = closes - d_close
        highs = opens + d_high
        lows = opens + d_low

        return {
            'id': trame_number, 'count': nb_el,
            'timeStart': timestamps, 'open': opens,
            'high': highs, 'low': lows, 'close': closes
        }
