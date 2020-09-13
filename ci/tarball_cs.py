#!/usr/bin/python3
#
#  Compute the sha256 hash for a given file, optionally update a file
#  by substituting $checksum$ with said sha256 hash.

import argparse
import hashlib

def get_args():
    """ Return parsed arg_parser instance."""

    parser = argparse.ArgumentParser(
            description="opencpn tarball checksumming tool (path).")
    parser.add_argument(
       'tarball', metavar='tarball', type=argparse.FileType('r'),
       help='tarball to comput checksum of.')
    parser.add_argument(
       '-m', '--metadata', metavar='metadata',
       help='Metadata xml to update with checksum (path)')
    parser.add_argument(
       '-s', '--silent', action='store_true', default=False)
    return parser.parse_args();

def compute_cs(path):
    """ Return sha256 hexdigest for file living in path. """

    sha256_hash = hashlib.sha256()
    with open(path, "rb") as f:
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
    return sha256_hash.hexdigest()

def substitute_cs(metadata, cs):
    """ Replace $checksum$ in metadata (a path) with cs. """

    try:
        with open(metadata) as f:
            contents = f.read();
        with open(metadata, "w") as f:
            f.write(contents.replace("$checksum$", cs));
    except IOError as e:
        print( "Error accessing file %s: %s" % (metadata, e))


def main():
    """ Indeed: main function."""

    args = get_args()
    cs = compute_cs(args.tarball.name)
    if not args.silent:
        print(cs + "  " + args.tarball.name)
    if not args.metadata:
        return
    substitute_cs(args.metadata, cs)


if __name__ == '__main__':
    main()
