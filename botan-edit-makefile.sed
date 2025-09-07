/LIB_LINKS_TO/s/\=/\= -L\/opt\/local\/lib\/libomp -lomp -L\/opt\/local\/libexec\/boost\/1\.88\/lib /
/LIB_LINKS_TO/s/ -lsqlite3/ -L\/opt\/local\/lib -lsqlite3 /
/LIB_LINKS_TO/s/system\ /system-mt\ /g
/ABI_FLAGS/s/fopenmp/Xpreprocessor -fopenmp/
/^all:/s/docs tests/tests docs/
