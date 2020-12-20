#!/usr/bin/python3
#
#  Compute the sha256 hash for a given file.

import hashlib
import sys

def compute_cs(path):
    """ Return sha256 hexdigest for file living in path. """

    sha256_hash = hashlib.sha256()
    with open(path, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def main():
    """ Indeed: main function."""

    cs = compute_cs(sys.argv[1])
    print(cs + "  " + sys.argv[1])

if __name__ == '__main__':
    main()
