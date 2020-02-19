#!/usr/bin/env bash -v

#./configure.py --prefix=/opt/local --without-unaligned-mem --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=2.7 --with-sphinx --enable-modules=pkcs11 --cc-abi-flags='-maes -mpclmul -mrdrnd -msse2 -mssse3 -msse4 -msse4.2 -Os -Ofast -I/opt/local/include'

./configure.py --debug --debug-mode --prefix=/opt/local --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-openmp --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=2.7 --with-sphinx --with-pdf --cc-abi-flags='-march=native -g -Os -Ofast -I/opt/local/include -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk' 2>&1 | tee conf-out.txt

gsed -f botan-edit-makefile.sed < Makefile > t.mak
mv t.mak Makefile
