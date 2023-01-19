#!/usr/bin/env bash -v

#CXXFLAGS="-std=c++20 -stdlib=libc++ -O3 -I/opt/local/libexec/boost/1.81/include -I/opt/local/include -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
CXXFLAGS="-std=c++20 -stdlib=libc++ -O3 -D_LIBCPP_DISABLE_AVAILABILITY -I/opt/local/libexec/boost/1.81/include -I/opt/local/include"
CPLUS_INCLUDE_PATH="/opt/local/libexec/boost/1.81/include"
LIBRARY_PATH="/opt/local/libexec/boost/1.81/lib:/opt/local/lib"
DISABL="-D_LIBCPP_DISABLE_AVAILABILITY"
BOTANLIBS="-lboost_thread-mt -lboost_random-mt -lboost_exception-mt -lboost_filesystem-mt -lboost_system-mt"

#./configure.py --prefix=/opt/local --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-openssl --with-boost --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=3.9 --with-sphinx --with-pdf --cc-bin='/opt/local/bin/clang++-mp-12' --cc-abi-flags='-march=native -Os -Ofast -I/opt/local/include' 2>&1 | tee conf-out.txt

./configure.py --prefix=/opt/local --with-boost --cc-bin='/opt/local/bin/clang++-mp-15' --extra-cxxflags='-I/opt/local/libexec/llvm-15/include' --ldflags='-L/opt/local/libexec/llvm-15/lib -Wl,-rpath,/opt/local/libexec/llvm-15/lib' --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-commoncrypto --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=3.9 --with-sphinx --with-pdf --system-cert-bundle=/opt/local/share/curl/curl-ca-bundle.crt --cc-abi-flags='-march=native'  2>&1 | tee conf-out.txt

gsed -f botan-edit-makefile.sed < Makefile > t.mak
mv t.mak Makefile
