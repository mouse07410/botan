#!/usr/bin/env bash -v

CXXFLAGS="-Ofast -Os -I/opt/local/include -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"

#./configure.py --prefix=/opt/local --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=3.9 --with-sphinx --with-pdf --cc-bin='/opt/local/bin/clang++-mp-12' --cc-abi-flags='-march=native -Os -Ofast -I/opt/local/include' 2>&1 | tee conf-out.txt

./configure.py --prefix=/opt/local --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-commoncrypto --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=3.9 --with-sphinx --with-pdf --system-cert-bundle=/opt/local/share/curl/curl-ca-bundle.crt --cc-abi-flags='-march=native'  2>&1 | tee conf-out.txt

gsed -f botan-edit-makefile.sed < Makefile > t.mak
mv t.mak Makefile
