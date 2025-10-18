# syntax=docker/dockerfile:1

# ---- Build stage ----
FROM rust:1-alpine AS build

# Install build dependencies
RUN apk add --no-cache musl-dev openssl-dev

# Create a new empty shell project
WORKDIR /app
COPY . .

# Build for release
RUN cargo build --release

# ---- Runtime stage ----
FROM alpine:latest

# OCI labels
LABEL org.opencontainers.image.title="tic2mqtt"
LABEL org.opencontainers.image.description="Rust app to read Enedis TIC serial data and export to MQTT for Home Assistant."
LABEL org.opencontainers.image.authors="Axel LORENTE <contact@passific.fr>"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.source="https://github.com/passific/tic2mqtt"

# Install runtime dependencies
RUN apk add --no-cache openssl

# Copy the binary
COPY --from=build /app/target/release/tic2mqtt /usr/local/bin/

# Create non-root user
RUN adduser -D tic2mqtt
USER tic2mqtt

# Environment variables with defaults
ENV MQTT_SERVER="tcp://localhost:1883" \
    MQTT_CLIENT_ID="tic2mqtt_client" \
    MQTT_USER="" \
    MQTT_PASS="" \
    TIC_MODE="historique" \
    SERIAL_PORT="/dev/ttyUSB0"

# Default command
ENTRYPOINT ["/usr/local/bin/tic2mqtt"]