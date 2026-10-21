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
import base64
import json
import time
import uuid

__all__ = [
    "Exness",
]


class ExnessException(ChartException):
    pass


class Exness(BaseChart):
    DFLen=1000
    HttpUrl="https://rtapi-jb.bardaykeydayoatlaweatbar.com/rtapi/exot/real1/v2/accounts/{accountId}3/instruments/{symbol}/candles?time_frame={seconds}&count=-{dflen}&price=bid"
    Headers = {
        "accept": "application/json, text/plain, */*",
        "accept-language": "en-US,en;q=0.9",
        "authorization": "",
        "priority": "u=1, i",
        "sec-ch-ua": '"Chromium";v="148", "Google Chrome";v="148", "Not/A)Brand";v="99"',
        "sec-ch-ua-mobile": "?0",
        "sec-ch-ua-platform": '"Linux"',
        "sec-fetch-dest": "empty",
        "sec-fetch-mode": "cors",
        "sec-fetch-site": "cross-site",
        "sec-fetch-storage-access": "active",
        "Referer": "https://my.exness.com/",
    }
    PingTimeout = 5
    WsUrl = "ASSIGNED_ENPOINT_URL"


    AuthToken = 'YOUR_EXNESS_TOKEN'

    def __init__(self, symbol: Symbols = Symbols.XAUUSD, period: Periods = Periods.m10):
        super().__init__(symbol, period)
        self.headers = Exness.Headers
        Exness.check_jwt_expiry()
        self.headers["Authorization"] = f"Bearer {Exness.AuthToken}"

    @staticmethod
    def check_jwt_expiry() -> None:
        token = Exness.AuthToken
        try:
            parts = token.split(".")
            if len(parts) != 3:
                raise ValueError("Invalid JWT token format.")
            payload_base64 = parts[1]
            payload_base64 += "=" * (-len(payload_base64) % 4)
            payload_bytes = base64.urlsafe_b64decode(payload_base64)
            payload = json.loads(payload_bytes.decode("utf-8"))
            if "exp" not in payload:
                raise ExnessException("missing 'exp' in jwt, check manually")
            exp_epoch = payload["exp"]
            current_epoch = time.time()
            if exp_epoch <= current_epoch:
                raise ExnessException("JWT Token has expired. Please manually update the token.")
        except Exception as e:
            if isinstance(e, ExnessException):
                raise e
            raise ExnessException(f"Failed to parse or validate token: {repr(e)}")

    def fetch_historical_data(self) -> pd.DataFrame:
        url = self.HttpUrl.format(symbol=self.symbol.name, seconds=self.period.period_value,dflen=Exness.DFLen)
        data = requests.get(url, headers=self.headers).json()
        if "code" in data:
            raise ExnessException(f"cannot fetch data {data['message']}")
        if len(data['price_history'])==0:
            raise Exception('empty response. maybe len too long')
        df = pd.DataFrame(data["price_history"]).rename(columns={"t": "d"})
        df = df[["d", "o", "h", "l", "c"]]
        df.index = (df.pop("d") / 1000).astype(np.int64)
        return df

    async def send_ping(self) -> None:
        while self.running:
            await self.ws.send(json.dumps(x := {"ping": {"timestamp_ms": int(time.time() * 1000)}, "id": str(uuid.uuid4())}))
            await asyncio.sleep(Exness.PingTimeout)

    async def get_ws_connection(self):
        headers = {
            "accept-language": "en-US,en;q=0.9",
            "cache-control": "no-cache",
            "pragma": "no-cache",
            "User-Agent":"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/148.0.0.0 Safari/537.36"
        }
        self.ws = await websockets.connect(self.WsUrl, additional_headers=headers, subprotocols=["exness-ws-protocol", Exness.AuthToken])
        self._start_event.set()
        asyncio.create_task(self.send_ping())
        await self.ws.send(json.dumps({"subscribe": {"event": "ticks", "instruments": [self.symbol.name]}, "id": str(uuid.uuid4())}))
        return self.ws

    async def parse_ws_message(self, _, msg: str) -> Optional[Tuple[int, float]]:
        j = json.loads(msg)
        if j.get("i") and j.get("b") and j.get("t") and j.get("a"):
            return (
                int(int(j["t"]) / 1000),
                (float(j["a"]) + float(j["b"])) / 2.0,
            )
