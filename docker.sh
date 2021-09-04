#!/bin/sh

docker build -t solana-llvm .
docker run -v .:/work solana-llvm
