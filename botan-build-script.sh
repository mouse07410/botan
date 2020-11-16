#!/bin/bash
sudo rm -rf /opt/local/include/botan-2/* || true
sudo rm -rf /opt/local/lib/libbotan-2.*  || true

rm -f build/doc.stamp 
rm -f libbotan-*
make clean && make -j 4 all 2>&1 | tee make-out.txt && DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:." ./botan-test --test-threads=0 --pkcs11-lib=/opt/local/lib/softhsm/libsofthsm2.so 2>&1 | tee test-out.txt && sudo make install && sudo chown -R uri * && cp build/docs/handbook/botan.pdf ~/Documents/botan-2.pdf  && rm -f build/doc.stamp

date
