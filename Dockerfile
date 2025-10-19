# syntax=docker/dockerfile:1


# ---- Build stage ----
FROM alpine AS build

# Install build tools and dependencies
RUN apk add --no-cache cmake g++ make git openssl-dev

# Install Paho MQTT C (from repo) and build Paho MQTT C++ from source
RUN apk add --no-cache paho-mqtt-c-dev

# Build and install Paho MQTT C++ in /deps
WORKDIR /deps
RUN git clone --depth 1 --branch v1.2.0 https://github.com/eclipse/paho.mqtt.cpp.git
WORKDIR /deps/paho.mqtt.cpp
RUN cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SHARED=ON -DPAHO_WITH_SSL=ON
RUN make -C build -j$(nproc)

# Install built Paho MQTT C++ library and headers to /usr/local for app build
RUN make -C build install


# Build the tic2mqtt app
WORKDIR /app
COPY . /app
RUN mkdir -p build && cd build && \
    cmake .. && \
    cmake --build .

# Minimal runtime image
FROM alpine

# OCI labels
LABEL org.opencontainers.image.title="tic2mqtt"
LABEL org.opencontainers.image.description="C++ app to read Enedis TIC serial data and export to MQTT for Home Assistant."
LABEL org.opencontainers.image.authors="Axel LORENTE <contact@passific.fr>"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.source="https://github.com/passific/tic2mqtt"

# Install runtime dependencies
RUN apk add --no-cache libstdc++ openssl paho-mqtt-c

# Copy built app
COPY --from=build /app/build/tic2mqtt /tic2mqtt

# Copy only Paho MQTT C++ libraries
COPY --from=build /usr/local/lib/libpaho-mqttpp3* /usr/lib/


CMD ["/tic2mqtt"]
