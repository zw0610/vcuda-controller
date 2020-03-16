# only libcuda.so is needed
FROM nvidia/cuda:10.1-devel-ubuntu18.04

RUN apt update && apt install -y --no-install-recommends \
  cmake libvdpau-dev && \
  rm -rf /var/lib/apt/lists/*

COPY . /root/vcuda/

RUN cd /root/vcuda && mkdir build && \
    cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)
