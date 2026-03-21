FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies + GCC trunk via Ubuntu Toolchain PPA (gcc-snapshot = nightly GCC trunk)
RUN apt-get update && apt-get install -y \
        software-properties-common \
        wget curl git vim less \
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y gcc-snapshot \
    && rm -rf /var/lib/apt/lists/*

# Put GCC trunk first on PATH
ENV PATH="/usr/lib/gcc-snapshot/bin:${PATH}"
ENV LD_LIBRARY_PATH="/usr/lib/gcc-snapshot/lib64:/usr/lib/gcc-snapshot/lib:${LD_LIBRARY_PATH}"

# Confirm version at build time
RUN g++ --version

WORKDIR /examples

COPY examples/ .
RUN chmod +x run_all.sh

# Welcome message
RUN echo 'echo ""' >> /root/.bashrc && \
    echo 'echo "=== GCC Trunk C++ Special Members Lab ==="' >> /root/.bashrc && \
    echo 'echo "  g++ version: $(g++ --version | head -1)"' >> /root/.bashrc && \
    echo 'echo "  Run: ./run_all.sh   → compile and run all examples"' >> /root/.bashrc && \
    echo 'echo "  Or:  make && ./bin/<example>"' >> /root/.bashrc && \
    echo 'echo ""' >> /root/.bashrc

CMD ["/bin/bash"]
