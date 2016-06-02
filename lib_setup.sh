#!/bin/bash
git submodule init
git submodule update

# google benchmark
cd lib/benchmark
mkdir build
cd build
cmake ../
make
cd ../../..

# hayai
cd lib/hayai
mkdir build
cd build
cmake ../
make
cd ../../..
