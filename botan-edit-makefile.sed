/LIB_LINKS_TO/s/\=/\= -L\/opt\/local\/lib\/libomp -lomp -L\/opt\/local\/libexec\/boost\/1\.81\/lib /
/LIB_LINKS_TO/s/system\ /system-mt\ /g
/ABI_FLAGS/s/fopenmp/Xpreprocessor -fopenmp/
/^all:/s/docs tests/tests docs/
