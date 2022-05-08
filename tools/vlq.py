import unittest

def vlq(x: int) -> bytearray:
    """
    Variable-Length Quantity encoding
    """
    bs = []
    while x > 0:
        bs.append(x % 128)
        x = x >> 7
    if len(bs) == 0:
        bs = [0]
    for i in range(1, len(bs)):
        bs[i] = bs[i] + 128
    bs.reverse()
    return bytearray(bs)

class Test_vlq(unittest.TestCase):
    def test_zero(self):
        self.assertEqual(vlq(0), bytearray([0x00]))
    def test_127(self):
        self.assertEqual(vlq(127), bytearray([0x7F]))
    def test_128(self):
        self.assertEqual(vlq(128), bytearray([0x81, 0x00]))
    def test_8192(self):
        self.assertEqual(vlq(8192), bytearray([0xC0, 0x00]))
    def test_16383(self):
        self.assertEqual(vlq(16383), bytearray([0xFF, 0x7F]))
    def test_16384(self):
        self.assertEqual(vlq(16384), bytearray([0x81, 0x80, 0x00]))

if __name__ == "__main__":
    unittest.main()
