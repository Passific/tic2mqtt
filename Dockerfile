# syntax=docker/dockerfile:1


# ---- Build stage ----
FROM alpine AS build

# Install build tools and dependencies
RUN apk add --no-cache cmake g++ make git openssl-dev


# Install Paho MQTT C (from repo) and build Paho MQTT C++ from source
RUN apk add --no-cache paho-mqtt-c-dev git cmake g++ make openssl-dev
# Build and install Paho MQTT C++
WORKDIR /tmp
RUN git clone --depth 1 --branch v1.2.0 https://github.com/eclipse/paho.mqtt.cpp.git \
    && cd paho.mqtt.cpp \
    && cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SHARED=ON -DPAHO_WITH_SSL=ON
RUN cd paho.mqtt.cpp && make -C build -j$(nproc)

# Install to a custom location to make copying easier
RUN cd paho.mqtt.cpp && make -C build install DESTDIR=/paho-install


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
RUN apk add --no-cache libstdc++ openssl

# Copy built app
COPY --from=build /app/build/tic2mqtt /tic2mqtt

# Copy Paho MQTT C and C++ libraries
COPY --from=build /paho-install/usr/local/lib/ /usr/local/lib/
COPY --from=build /paho-install/usr/local/include/ /usr/local/include/

# Set library path for dynamic linker
ENV LD_LIBRARY_PATH=/usr/local/lib

CMD ["/tic2mqtt"]
