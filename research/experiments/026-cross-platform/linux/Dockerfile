FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      autoconf automake clang lld llvm gcc git libmimalloc-dev libtool \
      make pkg-config python3 time \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /work
