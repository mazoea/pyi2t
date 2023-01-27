# Overview

This repository provides interface that wraps functionality from `image_to_text` (written in C++) to be possible to be called from python. This is used for example in `te-server` project. 

The interface consists of:
- Source files written in C++ - defines which parts of code will be exposed into python
- Source files written in python - encapsulates it to python

# Testing

Use `PYI2T_BINDIR` environmental variable to point to the directory with the python bindings binaries.

# Exposing symbols from libs (in linux)
- all function are hidden by default
- in case you want to expose certain function, add the function name into `libcode.version` file under `global:` functions