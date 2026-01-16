################################################################################
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
################################################################################


from typing import List, Tuple

class I2CBinGenerator:
    """
    I2CBinGenerator
    ----------------
    This class provides a flexible way to parse a text file containing I2C register configuration data and generate a binary output file in a custom format.

    Features:
    - Parses lines containing hexadecimal or decimal byte values, skipping comments and empty lines.
    - For normal entries (first byte < 0x80): drops the first byte and groups consecutive entries of the same length, writing a [count][length][data] header for each group.
    - For special entries (first byte >= 0x80): writes a header [0xFF,0xFF,0xFF,0xFF,0x04,<4-byte subtraction>] followed by the remaining bytes, with no count/length header.
    - Handles out-of-range values and supports both hex (with or without 0x) and decimal formats.
    - Utility methods are static for easy reuse and clarity.
    - Comments and whitespace in the input file are ignored.

    Usage:
        gen = I2CBinGenerator()
        gen.generate_bin('input.txt', 'output.bin')

    The output file will contain the processed binary data according to the rules above, suitable for firmware or hardware configuration.
    """

    def __init__(self):
        pass

    @staticmethod
    # identifier for checking if a string is a valid hex representation
    def check_hex_str(s: str) -> bool:
        s = s.strip()
        return len(s) > 0 and all(c in '0123456789abcdefABCDEF' for c in s)


    @staticmethod
    # function to parse the hexadecimal or decimal byte
    def parse_bytes(tok: str):
        # trim the spaces
        t = tok.strip()
        if not t:
            # default to None if empty
            return None
        # logic to parse the hexadecimal or decimal byte
        try:
            # 0 x FF to handle out of range values
            if t.lower().startswith('0x'):
                return int(t, 16) & 0xFF
            if I2CBinGenerator.check_hex_str(t):
                return int(t, 16) & 0xFF
            # fallback to decimal as a default case
            return int(t) & 0xFF
        except ValueError:
            return None


    @staticmethod
    # function to clean up lines by removing comments and trailing commas (comment removal function)
    def clean_line(line: str) -> str:
        line = line.strip()
        if '//' in line:
            line = line.split('//', 1)[0]
        return line.rstrip(',').strip()


   # Extract entries from the file
    def extract_entries(self, filename: str):
        # List for storing processed byte entries
        entries = []

        with open(filename, 'r', encoding='utf-8', errors='ignore') as f:
            # processing for each line in the file
            for raw in f:
                line = raw.strip()
                if not line:
                    continue
                # whole line commented
                if line.startswith('//'):
                    continue

                line = self.clean_line(line)
                if not line:
                    continue

                # split by comma and remove empty parts
                parts = [p for p in (t.strip() for t in line.split(',')) if p]
                if not parts:
                    continue

                # parse the extracted parts of hexadecimal or decimal bytes and store them in a List
                vals: List[int] = []
                for p in parts:
                    b = self.parse_bytes(p)
                    if b is not None:
                        vals.append(b)

                if not vals:
                    continue

                first = vals[0]

                if first == 0x00:
                    continue

                # This is the special case starting with 0x80 or greater
                if first >= 0x80:
                    sub_bytes = (first - 0x80) & 0xFFFFFFFF
                    prefix = [0xFF, 0xFF, 0xFF, 0xFF, 0x04] + list(sub_bytes.to_bytes(4, 'little'))
                    entry = prefix + vals[1:]
                else:
                    # normal case: drop the first byte, keep the rest
                    entry = vals[1:]

                # store the processed entry
                entries.append(entry)

        # return just the entries
        return entries


    # Function to generate the binary file from the entries
    def generate_bin(self, input_file: str, output_file: str):
        entries = self.extract_entries(input_file)
        with open(output_file, 'wb') as f:
            # Buffer to hold consecutive "normal" entries for grouping
            normal_entries_buffer: List[List[int]] = []

            # Helper function to write the buffered normal entries
            def write_normal_buffer():
                if not normal_entries_buffer:
                    return

                # Group the buffered entries by length (two-pointer approach)
                groups: List[Tuple[int, int]] = []
                if normal_entries_buffer:
                    prev = len(normal_entries_buffer[0])
                    cnt = 1
                    for entry in normal_entries_buffer[1:]:
                        L = len(entry)
                        if L == prev:
                            cnt += 1
                        else:
                            groups.append((cnt, prev))
                            prev = L
                            cnt = 1
                    groups.append((cnt, prev))

                # Write the grouped normal entries to the file
                idx = 0
                for cnt, blen in groups:
                    f.write(cnt.to_bytes(4, 'little'))
                    f.write(blen.to_bytes(1, 'little'))
                    for _ in range(cnt):
                        # trim or pad to expected length
                        row = (normal_entries_buffer[idx][:blen] + [0] * blen)[:blen]
                        f.write(bytes(row))
                        idx += 1

                # Clear the buffer after writing
                normal_entries_buffer.clear()

            for entry in entries:
                if len(entry) >= 5 and entry[:4] == [0xFF,0xFF,0xFF,0xFF]:
                    write_normal_buffer()
                    f.write(bytes(entry))
                else:
                    normal_entries_buffer.append(entry)

            write_normal_buffer()
