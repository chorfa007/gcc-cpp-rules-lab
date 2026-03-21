FROM --platform=linux/amd64 ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# Build dependencies for GCC from source
RUN apt-get update && apt-get install -y \
        build-essential \
        bison flex \
        libgmp-dev libmpfr-dev libmpc-dev libisl-dev \
        wget curl git vim less make \
        libboost-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone GCC trunk (reflection merged into master Jan 19 2026)
RUN git clone https://gcc.gnu.org/git/gcc.git --depth=1 --single-branch /gcc-src

# Build GCC with C++26 reflection support
RUN mkdir /gcc-build && cd /gcc-build && \
    /gcc-src/configure \
        --prefix=/opt/gcc \
        --enable-languages=c,c++ \
        --disable-multilib \
        --disable-bootstrap \
    && make -j$(nproc) \
    && make install-strip \
    && rm -rf /gcc-build /gcc-src

# Put our GCC first on PATH
ENV PATH="/opt/gcc/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/gcc/lib64:/opt/gcc/lib"

# Confirm version and reflection flag at build time
RUN g++ --version
RUN echo '#include <meta>' > /tmp/test.cpp && \
    g++ -std=c++26 -freflection -c /tmp/test.cpp -o /dev/null && \
    echo "✓ -freflection supported" && rm /tmp/test.cpp

WORKDIR /examples

COPY examples/ .
RUN chmod +x run_all.sh

# Welcome message
RUN echo 'echo ""' >> /root/.bashrc && \
    echo 'echo "=== GCC Trunk · P2996 Reflection for C++26 Lab ==="' >> /root/.bashrc && \
    echo 'echo "  g++ version : $(g++ --version | head -1)"' >> /root/.bashrc && \
    echo 'echo "  flags       : -std=c++26 -freflection"' >> /root/.bashrc && \
    echo 'echo "  Run all     : ./run_all.sh"' >> /root/.bashrc && \
    echo 'echo "  Or          : make && ./bin/<example>"' >> /root/.bashrc && \
    echo 'echo ""' >> /root/.bashrc

CMD ["/bin/bash"]
