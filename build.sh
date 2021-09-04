#!/bin/sh

cd /emsdk && . ./emsdk_env.sh
cd /work
mkdir -p build
cd build
emcmake cmake ..
emmake make
