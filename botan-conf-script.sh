#!/usr/bin/env bash -v


#./configure.py --prefix=/opt/local --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-openmp --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=2.7 --with-sphinx --with-pdf --cc-bin='/opt/local/bin/clang++-mp-8.0' --cc-abi-flags='-march=native -Os -Ofast -I/opt/local/include' 2>&1 | tee conf-out.txt

./configure.py --prefix=/opt/local --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-openmp --with-commoncrypto --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=2.7 --with-sphinx --with-pdf --cc-abi-flags='-march=native -O3 -I/opt/local/include' 2>&1 | tee conf-out.txt

gsed -f botan-edit-makefile.sed < Makefile > t.mak
mv t.mak Makefile
