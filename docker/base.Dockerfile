### Base environment container ###
# Get the base Ubuntu image from Docker Hub
FROM ubuntu:noble as base

ARG DEBIAN_FRONTEND=noninteractive

# Update the base image and install build environment
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    curl \
    httpie \
    libboost-log-dev \
    libboost-regex-dev \
    libboost-system-dev \
    libgmock-dev \
    libgtest-dev \
    netcat-openbsd \
    lcov \
    gcovr \
    nlohmann-json3-dev \
    bc