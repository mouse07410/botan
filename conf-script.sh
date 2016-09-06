#!/usr/bin/env bash -v

./configure.py --prefix=/opt/local --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=2.7 --with-sphinx --cc=clang --enable-modules=pkcs11 --cc-abi-flags='-maes -mpclmul -mrdrnd -msse2 -mssse3 -msse4 -msse4.2 -Os -Ofast -I/opt/local/include' --with-external-includedir='/Users/ur20980/src/pkcs11-base/'
