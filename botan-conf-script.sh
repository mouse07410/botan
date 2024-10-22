#!/usr/bin/env bash -v

#CXXFLAGS="-std=c++20 -stdlib=libc++ -O3 -I/opt/local/libexec/boost/1.81/include -I/opt/local/include -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk"
CPLUS_INCLUDE_PATH="/opt/local/libexec/boost/1.81/include"
LIBRARY_PATH="/opt/local/libexec/boost/1.81/lib:/opt/local/lib"
DISABL="-D_LIBCPP_DISABLE_AVAILABILITY"
BOTANLIBS="-lboost_thread-mt -lboost_random-mt -lboost_exception-mt -lboost_filesystem-mt -lboost_system-mt"

CXXFLAGS="-std=c++20 -stdlib=libc++ -O3 ${DISABL} -I/opt/local/libexec/boost/1.81/include -I/opt/local/include"

# Need Macports-installed Clang-18 to build Botan-3 with SDK from XCode-13.2.1
#./configure.py --prefix=/opt/local --program-suffix=3 --with-boost --cc-bin='/opt/local/bin/clang++-mp-18' --extra-cxxflags='-I/opt/local/libexec/llvm-18/include' --ldflags='-L/opt/local/libexec/llvm-18/lib -Wl,-rpath,/opt/local/libexec/llvm-18/lib' --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-commoncrypto --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=3.11 --with-sphinx --with-pdf --system-cert-bundle=/opt/local/share/curl/curl-ca-bundle.crt --cc-abi-flags='-march=native'  2>&1 | tee conf-out.txt
# Need Macports-installed Clang-16 to build Botan-3 with SDK from XCode-13.2.1
./configure.py --prefix=/opt/local --program-suffix=3 --with-boost --cc-bin='/opt/local/bin/clang++-mp-16' --extra-cxxflags='-I/opt/local/libexec/llvm-16/include' --ldflags='-L/opt/local/libexec/llvm-16/lib -Wl,-rpath,/opt/local/libexec/llvm-16/lib' --with-os-features=security_framework,apple_keychain,commoncrypto,getentropy --with-commoncrypto --with-lzma --with-bzip2 --with-zlib --with-sqlite3 --with-python-version=3.11 --with-sphinx --with-pdf --system-cert-bundle=/opt/local/share/curl/curl-ca-bundle.crt --cc-abi-flags='-march=native'  2>&1 | tee conf-out.txt

gsed -f botan-edit-makefile.sed < Makefile > t.mak
mv t.mak Makefile
