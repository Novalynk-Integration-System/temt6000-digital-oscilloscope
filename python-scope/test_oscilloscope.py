import struct
import unittest

from protocol import HEADER, MAGIC, Sample, StreamParser


class ParserTest(unittest.TestCase):
    @staticmethod
    def packet(sequence=42, first_us=1_000_000, rate=20_000, values=(100, 200, 300)):
        return HEADER.pack(MAGIC, 1, len(values), sequence, first_us, rate) + struct.pack(
            f"<{len(values)}H", *values
        )

    def test_valid_packet(self):
        parser = StreamParser()
        self.assertEqual(
            parser.feed(self.packet()),
            [Sample(42, 1_000_000, 100), Sample(43, 1_000_050, 200), Sample(44, 1_000_100, 300)],
        )

    def test_fragmented_packet(self):
        parser = StreamParser()
        packet = self.packet()
        self.assertEqual(parser.feed(packet[:10]), [])
        self.assertEqual(len(parser.feed(packet[10:])), 3)

    def test_resynchronizes_after_boot_log(self):
        parser = StreamParser()
        self.assertEqual(len(parser.feed(b"boot log\n" + self.packet())), 3)
        self.assertGreater(parser.malformed, 0)


if __name__ == "__main__":
    unittest.main()
