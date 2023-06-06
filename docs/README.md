# genconfig

This package is a DUNE DAQ repurposing of the genconfig package from
the ATLAS TDAQ. Users will interface with it by calling its
`daq_oks_codegen` CMake function. 

### daq_oks_codegen
Usage:
```
daq_oks_codegen(<oks schema filename1> ... [NAMESPACE ns] [DEP_PKGS pkg1 pkg2 ...])
```

`daq_oks_codegen` uses the genconfig package's application of the same
name to generate C++ and Python code from the OKS schema file(s)
provided to it.

Arguments:

  `<oks schema filename1> ...`: the list of OKS schema files to process from `<package>/schema/<package>`. 

 `NAMESPACE`: the namespace in which the generated C++ classes will be in. Defaults to `dunedaq::<package>`

 `DEP_PKGS`: if a schema file you've provided as an argument itself includes a schema file (or schema files) from one or more other packages, you need to supply the names of the packages as arguments to DEP_PKGS. 



