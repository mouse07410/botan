#!/bin/bash
botan-remove || true

rm -f build/doc.stamp 
rm -f libbotan-*
make clean && make -j 2 all 2>&1 | tee make-out.txt && ./botan-test --pkcs11-lib=/opt/local/lib/softhsm/libsofthsm2.so 2>&1 | tee test-out.txt && sudo make install && sudo chown -R uri * && cp build/docs/manual/botan.pdf ~/Documents/botan-2.pdf  && rm -f build/doc.stamp
