#!/bin/bash
botan-remove || true

#make clean && make -j 2 all && ./botan-test --pkcs11-lib=/opt/local/lib/softhsm/libsofthsm2.so && sudo make install && sudo chown -R uri * && cd build/docs/manual/ && make && cp botan.pdf ~/Documents/botan-2.0.pdf  && make clean && cd ../../.. && make clean

rm -f build/doc.stamp
make clean && make -j 2 all && ./botan-test --pkcs11-lib=/opt/local/lib/softhsm/libsofthsm2.so && sudo make install && sudo chown -R uri * && cp build/docs/manual/botan.pdf ~/Documents/botan-2.0.pdf  && make clean && rm -f build/doc.stamp
