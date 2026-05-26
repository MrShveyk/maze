FROM debian:bookworm-slim AS builder

RUN apt-get update \
 && apt-get install -y --no-install-recommends g++ make ca-certificates \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY Makefile ./
COPY src ./src

RUN make -j"$(nproc)"

FROM debian:bookworm-slim AS runtime

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        libstdc++6 \
 && rm -rf /var/lib/apt/lists/* \
 && useradd --system --uid 1000 --create-home --shell /usr/sbin/nologin maze

WORKDIR /app
COPY --from=builder /src/maze /app/maze

USER maze

EXPOSE 4321

ENTRYPOINT ["/app/maze"]
CMD ["-s"]
