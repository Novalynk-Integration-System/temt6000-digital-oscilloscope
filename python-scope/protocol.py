"""Serial protocol types and parser, kept independent from GUI dependencies."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Sample:
    sequence: int
    timestamp_us: int
    raw: int
    millivolts: int


def parse_sample(line: bytes) -> Sample | None:
    try:
        fields = line.decode("ascii").strip().split(",")
        if len(fields) != 5 or fields[0] != "D":
            return None
        return Sample(*(int(value) for value in fields[1:]))
    except (UnicodeDecodeError, ValueError):
        return None
