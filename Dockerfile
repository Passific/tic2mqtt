# syntax=docker/dockerfile:1


# ---- Build stage ----
FROM alpine:3.19 AS build

# Install build tools and dependencies
RUN apk add --no-cache cmake g++ make git openssl-dev boost-dev

# Install Paho MQTT C++
RUN apk add --no-cache paho-mqtt-c-dev paho-mqtt-cpp-dev


WORKDIR /app
COPY . /app
RUN mkdir -p build && cd build && \
    cmake .. && \
    cmake --build .

# Minimal runtime image
FROM alpine:3.19

# OCI labels
LABEL org.opencontainers.image.title="tic2mqtt"
LABEL org.opencontainers.image.description="C++ app to read Enedis TIC serial data and export to MQTT for Home Assistant."
LABEL org.opencontainers.image.authors="Axel LORENTE <contact@passific.fr>"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.source="https://github.com/passific/tic2mqtt"
RUN apk add --no-cache libstdc++ boost-system boost-thread openssl
COPY --from=build /app/build/tic2mqtt /tic2mqtt
CMD ["/tic2mqtt"]
