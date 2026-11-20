#!/usr/bin/env python3
"""Real-time serial oscilloscope for the TEMT6000 ESP32 firmware."""

from __future__ import annotations

import argparse
import queue
import sys
import threading
import time
from collections import deque
import numpy as np
import pyqtgraph as pg
import serial
from serial.tools import list_ports
from PySide6 import QtCore, QtWidgets

from protocol import Sample, parse_sample


class SerialReader(threading.Thread):
    def __init__(self, port: str, baud: int, output: queue.Queue[Sample]) -> None:
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.output = output
        self.stop_event = threading.Event()
        self.error: Exception | None = None
        self.malformed = 0

    def run(self) -> None:
        try:
            with serial.Serial(self.port, self.baud, timeout=0.2) as connection:
                connection.reset_input_buffer()
                while not self.stop_event.is_set():
                    line = connection.readline()
                    sample = parse_sample(line)
                    if sample is not None:
                        self.output.put(sample)
                    elif line:
                        self.malformed += 1
        except Exception as exc:  # reported by the UI timer
            self.error = exc


class ScopeWindow(QtWidgets.QMainWindow):
    def __init__(self, port: str, baud: int, window_seconds: float) -> None:
        super().__init__()
        self.setWindowTitle(f"TEMT6000 Digital Oscilloscope — {port}")
        self.resize(1100, 650)
        self.incoming: queue.Queue[Sample] = queue.Queue()
        capacity = max(1000, int(window_seconds * 1200))
        self.samples: deque[Sample] = deque(maxlen=capacity)
        self.window_seconds = window_seconds
        self.received = 0
        self.missing = 0
        self.resets = 0
        self.previous_sequence: int | None = None
        self.started = time.monotonic()
        self.arrival_times: deque[float] = deque()

        self.plot = pg.PlotWidget()
        self.plot.setLabel("bottom", "Time", "s")
        self.plot.setLabel("left", "ADC output", "mV")
        self.plot.showGrid(x=True, y=True, alpha=0.3)
        self.curve = self.plot.plot(pen=pg.mkPen("#00d8ff", width=2))
        self.setCentralWidget(self.plot)
        self.status = self.statusBar()

        self.reader = SerialReader(port, baud, self.incoming)
        self.reader.start()
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.refresh)
        self.timer.start(20)  # render at 50 FPS; serial still drains continuously

    @QtCore.Slot()
    def refresh(self) -> None:
        while True:
            try:
                sample = self.incoming.get_nowait()
            except queue.Empty:
                break
            if self.previous_sequence is not None:
                delta = (sample.sequence - self.previous_sequence) & 0xFFFFFFFF
                if 1 < delta < 0x80000000:
                    self.missing += delta - 1
                elif delta >= 0x80000000:
                    self.resets += 1
            self.previous_sequence = sample.sequence
            self.received += 1
            self.samples.append(sample)
            self.arrival_times.append(time.monotonic())

        if self.reader.error is not None:
            self.timer.stop()
            QtWidgets.QMessageBox.critical(self, "Serial error", str(self.reader.error))
            return
        if not self.samples:
            self.status.showMessage("Waiting for valid D,... samples")
            return

        latest_us = self.samples[-1].timestamp_us
        cutoff_us = latest_us - int(self.window_seconds * 1_000_000)
        visible = [sample for sample in self.samples if sample.timestamp_us >= cutoff_us]
        timestamps = np.fromiter((s.timestamp_us for s in visible), dtype=np.int64)
        use_mv = visible[-1].millivolts >= 0
        values = np.fromiter(
            ((s.millivolts if use_mv else s.raw) for s in visible), dtype=np.float64
        )
        self.plot.setLabel("left", "ADC output", "mV" if use_mv else "raw")
        self.curve.setData((timestamps - latest_us) / 1_000_000.0, values)
        now = time.monotonic()
        while self.arrival_times and self.arrival_times[0] < now - 1.0:
            self.arrival_times.popleft()
        elapsed = max(0.001, now - self.started)
        rate_1s = float(len(self.arrival_times))
        self.status.showMessage(
            f"received={self.received}  rate_1s={rate_1s:.0f} sample/s  "
            f"average={self.received / elapsed:.1f} sample/s  missing={self.missing}  "
            f"malformed={self.reader.malformed}  resets={self.resets}"
        )

    def closeEvent(self, event) -> None:  # noqa: N802 (Qt API)
        self.reader.stop_event.set()
        self.reader.join(timeout=1.0)
        event.accept()


def available_ports() -> str:
    ports = [port.device for port in list_ports.comports()]
    return ", ".join(ports) if ports else "none detected"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="serial port, for example COM7")
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--window", type=float, default=5.0, help="visible seconds")
    args = parser.parse_args()
    if not args.port:
        parser.error(f"--port is required (available: {available_ports()})")

    app = QtWidgets.QApplication(sys.argv)
    pg.setConfigOptions(antialias=False)
    window = ScopeWindow(args.port, args.baud, args.window)
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
