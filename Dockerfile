# only libcuda.so is needed
FROM nvidia/cuda:10.1-devel-ubuntu18.04

RUN apt update && apt install -y --no-install-recommends \
  cmake libvdpau-dev && \
  rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/zw0610/vcuda-controller.git && \
    cd vcuda-controller && mkdir build && \
    cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)