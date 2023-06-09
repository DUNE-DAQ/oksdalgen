# genconfig

This package is a DUNE DAQ repurposing of the genconfig package from
the ATLAS TDAQ. Users will interface with it by calling its
`daq_oks_codegen`. *Please note that as of June 7, 2023, if you add a class (or rename a class) in an OKS schema file which you pass to `daq_oks_codegen`, you will need to perform a [clean build](https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-buildtools/#useful-build-options) of your package.*

For documentation on `daq_oks_codegen`, please take a look at the [daq-cmake documentation](https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-cmake/).

