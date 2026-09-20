FROM debian:sid

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    binutils \
    ca-certificates \
    cmake \
    dpkg-dev \
    g++ \
    gcc \
    make \
    pkg-config \
    python3 \
    qt6-base-dev \
    libopencv-dev \
    libsdl2-dev \
    libsdl2-ttf-dev \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /work
