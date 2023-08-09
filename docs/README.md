# genconfig

This package is a DUNE DAQ repurposing of the genconfig package from
the ATLAS TDAQ. Users will interface with it by calling its
`daq_oks_codegen` CMake function, documented below. *Please note that as of June 7, 2023, if you add a class (or rename a class) in an OKS schema file which you pass to `daq_oks_codegen`, you will need to perform a [clean build](https://dune-daq-sw.readthedocs.io/en/latest/packages/daq-buildtools/#useful-build-options) of your package.*

### daq_oks_codegen
Usage:
```
daq_oks_codegen(<oks schema filename1> ... [NAMESPACE ns] [DEP_PKGS pkg1 pkg2 ...])
```

`daq_oks_codegen` uses the genconfig package's application of the same
name to generate C++ and Python code from the OKS schema file(s)
provided to it. To use it, you're required to call `find_package(genconfig REQUIRED)` earlier in your `CMakeLists.txt` file. 

Arguments:

  `<oks schema filename1> ...`: the list of OKS schema files to process from `<package>/schema/<package>`. 

 `NAMESPACE`: the namespace in which the generated C++ classes will be in. Defaults to `dunedaq::<package>`

 `DEP_PKGS`: if a schema file you've provided as an argument itself includes a schema file (or schema files) from one or more other packages, you need to supply the names of the packages as arguments to DEP_PKGS. 
 
Note that if you wish for your schema file to include another schema file, there are two additional things to keep in mind:

1. You'll also need to call `find_package` for the packages whose schema files get included
2. You'll need to add this snippet above the OKS class declarations in your schema file which looks like the following:
```
<include>
 <file path="schema/<some package name>/dunedaq.schema.xml"/>
</include>
```



