FROM debian:bookworm-slim

RUN apt update && apt install -y --no-install-recommends build-essential pkg-config libssl-dev ca-certificates && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN mkdir -p lab && gcc -D_GNU_SOURCE -std=c17 -O2 -Wall -Wextra -Isrc/include src/main.c src/control_T3C.c src/hash.c src/control_dict.c src/menu.c -o lab/a.out -lcrypto

ENTRYPOINT ["/app/lab/a.out"]
CMD ["-h"]
