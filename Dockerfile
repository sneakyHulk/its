# src: cpu, cuda, rocm
ARG src=cpu

FROM --platform=$BUILDPLATFORM ubuntu:noble AS cpu-base
ARG BUILDPLATFORM
RUN echo "I am running on $BUILDPLATFORM"

FROM nvidia/cuda:12.6.3-cudnn-devel-ubuntu24.04 AS cuda-base
FROM rocm/dev-ubuntu-24.04:6.2.4-complete AS rocm-base

FROM --platform=$BUILDPLATFORM ${src}-base AS image
ENV PYTHONUNBUFFERED 1

# https://apt.kitware.com/
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get -y --quiet --no-install-recommends install \
       ca-certificates \
       gpg \
       wget \
       software-properties-common \
    && test -f /usr/share/doc/kitware-archive-keyring/copyright || \
       wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null \
    && echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null \
    && apt-get update \
    && test -f /usr/share/doc/kitware-archive-keyring/copyright || \
       rm /usr/share/keyrings/kitware-archive-keyring.gpg \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get -y --quiet --no-install-recommends install \
       kitware-archive-keyring \
       cmake \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -fr /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

# install other dependencies
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get -y --quiet --no-install-recommends install \
       build-essential \
       gcc-14 \
       g++-14 \
       pkg-config \
       git \
       gdb \
       libopencv-dev \
       unzip \
       libeigen3-dev \
       libomp-dev \
       libtbb-dev \
       libgtk-3-dev \
       libmosquitto-dev \
       nlohmann-json3-dev \
       libssl-dev \
       libgpiod-dev \
       net-tools \
       telnet \
       iputils-ping \
       iproute2 \
       libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio libgstrtspserver-1.0-dev \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -fr /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

# Python install
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get -y --quiet --no-install-recommends install \
       python3 \
       python3-full \
       python3-pip \
       python-is-python3 \
       python-dev-is-python3 \
       python3-opencv \
       python3-opencv \
       python3-brotli \
       python3-numpy \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -fr /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*
RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

ARG src

# Install libtorch and pytorch
RUN if [ "$src" = "cpu" ]; then \
      python3 -m pip install --index-url https://pypi.python.org/simple torch torchvision torchaudio \
      && python3 -m pip install --index-url https://pypi.python.org/simple ultralytics pypylon \
      && wget --quiet -c https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcpu.zip --output-document libtorch.zip \
      && unzip libtorch.zip -d ~/src; \
    elif [ "$src" = "cuda" ]; then \
      python3 -m pip install --index-url https://download.pytorch.org/whl/cu124 torch torchvision torchaudio \
      && python3 -m pip install --index-url https://pypi.python.org/simple ultralytics pypylon \
      && wget --quiet -c https://download.pytorch.org/libtorch/cu124/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Bcu124.zip --output-document libtorch.zip \
      && unzip libtorch.zip -d ~/src; \
    elif [ "$src" = "rocm" ]; then \
      python3 -m pip install --index-url https://download.pytorch.org/whl/rocm6.2 torch torchvision torchaudio \
      && python3 -m pip install --index-url https://pypi.python.org/simple ultralytics pypylon \
      && wget --quiet -c https://download.pytorch.org/libtorch/rocm6.2/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Brocm6.2.zip --output-document libtorch.zip \
      && unzip libtorch.zip -d ~/src; \
    fi

ARG BUILDPLATFORM
# Install arm version: https://downloads-ctf.baslerweb.com/dg51pdwahxgw/3WTEu607sfl0dKNg69cNws/88cf3da4418aee38f2179e757ed9e2c7/pylon-8.0.1-16188_linux-aarch64_debs.tar.gz
# Install Pylon pylon 8.0.1: https://downloads-ctf.baslerweb.com/dg51pdwahxgw/2xjc1trdu02JMUFlzz8fR6/1a2fa4d4c54fdc1230ace817a6f2f37a/pylon-8.0.1-16188_linux-x86_64_debs.tar.gz
RUN if [ "$BUILDPLATFORM" = "linux/amd64" ]; then \
        wget --quiet https://downloads-ctf.baslerweb.com/dg51pdwahxgw/2xjc1trdu02JMUFlzz8fR6/1a2fa4d4c54fdc1230ace817a6f2f37a/pylon-8.0.1-16188_linux-x86_64_debs.tar.gz -O pylon.tar.gz \
        && tar -xzf pylon.tar.gz \
        && dpkg -i pylon*.deb \
        && apt-get update \
        && apt-get -y autoremove \
        && apt-get clean autoclean \
        && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*; \
     elif [ "$BUILDPLATFORM" = "linux/arm64" ]; then \
        wget --quiet https://downloads-ctf.baslerweb.com/dg51pdwahxgw/3WTEu607sfl0dKNg69cNws/88cf3da4418aee38f2179e757ed9e2c7/pylon-8.0.1-16188_linux-aarch64_debs.tar.gz -O pylon.tar.gz \
        && tar -xzf pylon.tar.gz \
        && dpkg -i pylon*.deb \
        && apt-get update \
        && apt-get -y autoremove \
        && apt-get clean autoclean \
        && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*; \
    fi


# Install eCAL
RUN add-apt-repository ppa:ecal/ecal-latest \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get -y --quiet --no-install-recommends install \
       ecal \
       protobuf-compiler \
       libprotobuf-dev \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

# 1. Line enables networking
# 2. Line enables zero copy mode
# 3. Line enables multi-bufferfing mode, to relax the publisher needs to wait for the subscriber to release the buffer.
# currently 10 buffer, number of buffers can be increased, but need more RAM
RUN awk -F"=" '/^network_enabled/{$2="= true"}1' /etc/ecal/ecal.ini | \
    awk -F"=" '/^memfile_zero_copy/{$2="= 1"}1' | \
    awk -F"=" '/^memfile_buffer_count/{$2="= 10"}1' > /etc/ecal/ecal.tmp && \
	rm /etc/ecal/ecal.ini && \
	mv /etc/ecal/ecal.tmp /etc/ecal/ecal.ini

ENV PYTHONPATH "${PYTHONPATH}:/src"
ENV CC gcc-14
ENV CXX g++-14

WORKDIR /src
