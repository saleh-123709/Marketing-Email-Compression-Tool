FROM debian:bookworm

RUN apt-get update && apt-get install -y \
    g++ \
    gzip \
    zip \
    brotli \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY main.cpp .

RUN g++ -std=c++17 -O2 main.cpp -o app

EXPOSE 8080

CMD ["./app"]
