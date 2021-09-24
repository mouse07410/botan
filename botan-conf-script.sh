#!/usr/bin/env bash -v

CXXFLAGS="-O1 -I/opt/local/include -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"

./configure.py --prefix=/opt/local --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-openmp --with-commoncrypto --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=3.9 --with-sphinx --with-pdf --cc-abi-flags='-march=native' --no-optimizations  2>&1 | tee conf-out.txt

gsed -f botan-edit-makefile.sed < Makefile > t.mak
mv t.mak Makefile
