import sys

MEMO_NORMAL_LACE = 0x80
MEMO_FINE_LACE = 0x81
MEMO_K_CARRIAGE = 0x02


class LaceCompiler:
    def __init__(self, lace_file):
        self.lines = []
        self.max_width = 0
        self._read_lace_file(lace_file)
        self._validate_lines()

    def _read_lace_file(self, lace_file):
        for raw_line in open(lace_file, 'r'):
            line = raw_line.rstrip('\n')
            self.lines.append(line)
            if len(line) > self.max_width:
                self.max_width = len(line)

    def _validate_lines(self):
        line_count = 0
        for line in self.lines:
            line_count += 1
            if ('<<' in line or '>>' in line or
                '<>' in line or
                '\\\\' in line or '//' in line):
                raise Exception("invalid stitch movement (double) in "
                                "line %d" % line_count)

    def _compile_line_for_type(self, line, char):
        if not char in line:
            return None
        res = [x==char for x in line]
        res_len = len(res)
        while res_len<self.max_width:
            res.append(False)
            res_len += 1
        return res

    def _compile_line(self, line):
        rows = []
        types = [('>',  MEMO_NORMAL_LACE),
                 ('<',  MEMO_NORMAL_LACE),
                 ('/', MEMO_FINE_LACE),
                 ('\\',  MEMO_FINE_LACE)]
        last_memo = None
        for char, memo in types:
            needles = self._compile_line_for_type(line, char)
            if needles:
                row = (memo, needles)
                rows.append(row)
                last_memo = memo

        if last_memo is None:
            return []

        empty_needles = [False for _ in range(self.max_width)]

        # make sure the amount of rows is pair, so the lace tool goes left
        # or the K carriage goes right if we had no action to stitches to move
        empty_row = (MEMO_K_CARRIAGE , empty_needles)
        if len(rows)%2 != 0:
            rows.append(empty_row)
        else:
            memo_row = (last_memo, empty_needles)
            rows.append(memo_row)
            rows.append(empty_row)

        print "  ", line
        for memo, needles in rows:
            print "%02X" % memo, \
                   "".join('*' if needle else '.' for needle in needles)
        return reversed(rows)

    def compile(self, raw_writer):
        for line in self.lines:
            rows = self._compile_line(line)
            for memo, needles in rows:
                raw_writer.add_row(memo, needles)


class RawWriter:
    def __init__(self, raw_file):
        self._raw_file = raw_file
        self._rows = []
        self._width = None

    def add_row(self, memo, needle_list):
        data = [memo]
        for needle in needle_list:
            data.append(0x00 if needle else 0xFF)
        self._rows.append(data)
        width = len(needle_list)
        if self._width and width != self._width:
            raise Exception("We don't accept rows of different sizes")
        else:
            self._width = width

    def write(self):
        self._f = open(self._raw_file, 'wb')
        self._write_header()
        for row in self._rows:
            self._write_row(row)
        self._f.close()

    def _write(self, uint8):
        self._f.write(chr(uint8))

    def _write_row(self, row):
        for stitch in row:
            self._write(stitch)

    def _write_header(self):
        self._write(0x80|(self._width>>8))
        self._write(self._width & 0xFF)
        height = len(self._rows)
        self._write(height>>8)
        self._write(height&0xFF)


def _test_raw_writter(out_raw):
    t = RawWriter(out_raw)
    t.add_row(MEMO_NORMAL_LACE, [True, False, True, False])
    t.add_row(MEMO_NORMAL_LACE, [False, True, False, True])
    t.add_row(MEMO_NORMAL_LACE, [True, False, True, False])
    t.add_row(MEMO_NORMAL_LACE, [False, True, False, True])
    t.write()

def main(in_lace, out_raw):
    lace = LaceCompiler(in_lace)
    raw_writer = RawWriter(out_raw)
    lace.compile(raw_writer)
    raw_writer.write()

if __name__ == '__main__':
    main(in_lace=sys.argv[1], out_raw=sys.argv[2])
