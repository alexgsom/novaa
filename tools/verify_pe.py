"""Check that an artifact really is a Windows x64 PE32+ executable.

Windows executable metadata inspection only: *not* a code-signing or malware
verification tool. Run on CI-produced artifacts before publication.
"""
import argparse
import pathlib
import struct
import sys

class PEError(ValueError):
    pass


def inspect_pe(data: bytes):
    if len(data) < 0x40 or data[:2] != b'MZ':
        raise PEError('Missing DOS MZ header')
    offset = struct.unpack_from('<I', data, 0x3C)[0]
    if offset < 0x40 or offset + 24 > len(data) or data[offset:offset+4] != b'PE\0\0':
        raise PEError('Missing or invalid PE header')
    machine, number_of_sections = struct.unpack_from('<HH', data, offset+4)
    optional_size = struct.unpack_from('<H', data, offset+20)[0]
    opt = offset+24
    if optional_size < 70 or opt+optional_size > len(data):
        raise PEError('Truncated PE optional header')
    magic = struct.unpack_from('<H', data, opt)[0]
    subsystem = struct.unpack_from('<H', data, opt+68)[0]
    if machine != 0x8664:
        raise PEError(f'Expected AMD64 (0x8664), got 0x{machine:04x}')
    if magic != 0x20b:
        raise PEError(f'Expected PE32+ (0x20b), got 0x{magic:x}')
    if number_of_sections == 0:
        raise PEError('Executable has no sections')
    if subsystem not in (2, 3):
        raise PEError(f'Unexpected PE subsystem: {subsystem}')
    return {'arch': 'x86-64', 'subsystem': 'GUI' if subsystem == 2 else 'Console'}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('binary', nargs='+', type=pathlib.Path)
    parser.add_argument('--gui', action='append', default=[], help='filename expected to have a GUI subsystem')
    parser.add_argument('--console', action='append', default=[], help='filename expected to have a Console subsystem')
    args = parser.parse_args()
    for path in args.binary:
        info = inspect_pe(path.read_bytes())
        if path.name in args.gui and info['subsystem'] != 'GUI':
            raise PEError(f'{path}: expected GUI subsystem')
        if path.name in args.console and info['subsystem'] != 'Console':
            raise PEError(f'{path}: expected Console subsystem')
        print(f'PASS: {path}: {info["arch"]} {info["subsystem"]}')


if __name__ == '__main__':
    try:
        main()
    except (PEError, OSError) as err:
        print(f'PE verification failed: {err}', file=sys.stderr)
        sys.exit(1)
