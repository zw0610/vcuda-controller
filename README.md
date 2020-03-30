# vcuda-controller

Caicloud customized implementation of [vcuda-control](https://github.com/tkestack/vcuda-controller). 

We decoupled the CUDA Driver API hijack part from TKE stack, dding memory tracking system for accounting GPU memory usage.


## Design

Please refer the [Google Docs](https://docs.google.com/document/d/1UX6nEOVFCtyNGGrQLdYnDWVche98pbfhPDwBgYa5AP8/edit) for design.

## Build

```shell
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

For more details on build, please check the Dockerfile.