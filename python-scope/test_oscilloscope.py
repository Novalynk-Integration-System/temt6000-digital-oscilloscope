import unittest

from protocol import Sample, parse_sample


class ParserTest(unittest.TestCase):
    def test_valid_frame(self):
        self.assertEqual(parse_sample(b"D,42,1234567,2048,1650\n"), Sample(42, 1234567, 2048, 1650))

    def test_ignores_boot_log(self):
        self.assertIsNone(parse_sample(b"I (123) boot: ESP-IDF\n"))

    def test_ignores_bad_frame(self):
        self.assertIsNone(parse_sample(b"D,broken\n"))


if __name__ == "__main__":
    unittest.main()
