FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    gcc-14 \
    cmake \
    make \
    pkg-config \
    libsodium-dev \
    librocksdb-dev \
    liblz4-dev \
    libcurl4-gnutls-dev \
    libjavascriptcoregtk-4.1-dev \
    libgtest-dev \
    libbenchmark-dev \
    curl \
    nodejs \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
RUN git clone https://github.com/gritzko/librdx.git .

RUN sed -i '/POLLtest/d' abc/test/CMakeLists.txt

RUN mkdir build && cd build \
    && CC=gcc-14 cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_INET=ON .. \
    && make -j$(nproc) be-srv be-cli

COPY ci-app/ /src/ci-app/
COPY start.sh /src/start.sh
RUN chmod +x /src/start.sh

EXPOSE 8800 8801

CMD ["/src/start.sh"]
