#!/bin/bash
botan-remove || true

rm -f build/doc.stamp 
rm -f libbotan-*
make clean && make -j 4 all 2>&1 | tee make-out.txt && DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:." ./botan-test3 --test-threads=0 --pkcs11-lib=/opt/local/lib/softhsm/libsofthsm2.so 2>&1 | tee test-out.txt && sudo make install && sudo chown -R uri * && cp build/docs/handbook/botan.pdf ~/Documents/botan-3.pdf  && rm -f build/doc.stamp

date
