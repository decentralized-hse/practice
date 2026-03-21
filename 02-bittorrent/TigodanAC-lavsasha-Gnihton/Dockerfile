FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc libc6-dev libmaxminddb-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY dht_scanner.c filter_ips.c enrich_geo.c statistics.c ./

RUN gcc -O2 -Wall -Wextra -std=c11 -o dht_scanner dht_scanner.c && \
    gcc -O2 -Wall -Wextra -std=c11 -o filter_ips filter_ips.c && \
    gcc -O2 -Wall -Wextra -std=c11 -o enrich_geo enrich_geo.c -lmaxminddb && \
    gcc -O2 -Wall -Wextra -std=c11 -o statistics statistics.c -lm

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates libmaxminddb0 gnuplot-nox fonts-dejavu-core \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /build/dht_scanner /build/filter_ips /build/enrich_geo /build/statistics /app/
COPY entrypoint.sh /app/
RUN chmod +x /app/entrypoint.sh

ENTRYPOINT ["/app/entrypoint.sh"]
