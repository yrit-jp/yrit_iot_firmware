# ビルドステージ
FROM debian:13.2-slim AS builder

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    wget \
    git \
    make \
    ca-certificates && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /doxygen

RUN wget https://github.com/doxygen/doxygen/releases/download/Release_1_15_0/doxygen-1.15.0.linux.bin.tar.gz
RUN gunzip doxygen-1.15.0.linux.bin.tar.gz && tar xf doxygen-1.15.0.linux.bin.tar 
RUN cd doxygen-1.15.0 && \
    make install

RUN git clone https://github.com/jothepro/doxygen-awesome-css.git && \
    cd doxygen-awesome-css && \
    make install

# 実行ステージ
FROM debian:13.2-slim

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    graphviz \
    fonts-freefont-ttf && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /doxygen

# ビルドステージからインストール済みのファイルをコピー
COPY --from=builder /usr/local/share /usr/local/share
COPY --from=builder /usr/local/bin /usr/local/bin

COPY Doxyfile /
COPY publish.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/publish.sh

ENTRYPOINT ["publish.sh"]