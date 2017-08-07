#!/bin/bash
botan-remove || true
make clean && make -j 2 && ./botan-test --pkcs11-lib=/opt/local/lib/softhsm/libsofthsm2.so && sudo make install && sudo chown -R uri * && cd build/docs/manual/ && make && cp botan.pdf ~/SkyDrive/botan-2.0.pdf  && make clean && cd ../../.. && make clean
