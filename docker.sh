#!/bin/sh

docker build -t solana-llvm .
docker run -v ${PWD}:/work solana-llvm
