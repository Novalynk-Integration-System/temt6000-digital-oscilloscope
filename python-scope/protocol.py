"""Binary block protocol used by the continuous ADC firmware."""

from __future__ import annotations

import struct
from dataclasses import dataclass

MAGIC = b"TMT1"
HEADER = struct.Struct("<4sHHIqI")
VERSION = 1
MAX_SAMPLES = 64


@dataclass(frozen=True)
class Sample:
    sequence: int
    timestamp_us: int
    raw: int
    millivolts: int = -1


class StreamParser:
    def __init__(self) -> None:
        self.buffer = bytearray()
        self.malformed = 0

    def feed(self, data: bytes) -> list[Sample]:
        self.buffer.extend(data)
        result: list[Sample] = []
        while True:
            magic_index = self.buffer.find(MAGIC)
            if magic_index < 0:
                if len(self.buffer) > len(MAGIC) - 1:
                    del self.buffer[: -(len(MAGIC) - 1)]
                    self.malformed += 1
                break
            if magic_index:
                del self.buffer[:magic_index]
                self.malformed += 1
            if len(self.buffer) < HEADER.size:
                break
            _, version, count, sequence, first_us, rate_hz = HEADER.unpack_from(self.buffer)
            if version != VERSION or not 0 < count <= MAX_SAMPLES or rate_hz == 0:
                del self.buffer[0]
                self.malformed += 1
                continue
            packet_size = HEADER.size + count * 2
            if len(self.buffer) < packet_size:
                break
            raw_values = struct.unpack_from(f"<{count}H", self.buffer, HEADER.size)
            for index, raw in enumerate(raw_values):
                result.append(
                    Sample(
                        sequence=(sequence + index) & 0xFFFFFFFF,
                        timestamp_us=first_us + index * 1_000_000 // rate_hz,
                        raw=raw,
                    )
                )
            del self.buffer[:packet_size]
        return result
