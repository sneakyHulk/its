# src: cpu, cuda, rocm
ARG src=cpu

FROM --platform=$BUILDPLATFORM ubuntu:jammy AS cpu-base
ARG BUILDPLATFORM
RUN echo "I am running on $BUILDPLATFORM"
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive \
       apt-get -y --quiet --no-install-recommends install \
       ca-certificates \
       wget \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

RUN if [ "$BUILDPLATFORM" = "linux/amd64" ]; then \
      wget --quiet https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh -O miniconda.sh -q \
      && mkdir -p /opt \
      && bash miniconda.sh -b -p /opt/conda \
      && rm miniconda.sh \
      && ln -s /opt/conda/etc/profile.d/conda.sh /etc/profile.d/conda.sh \
      && echo ". /opt/conda/etc/profile.d/conda.sh" >> ~/.bashrc \
      && echo "conda activate base" >> ~/.bashrc \
      && find /opt/conda/ -follow -type f -name '*.a' -delete \
      && find /opt/conda/ -follow -type f -name '*.js.map' -delete \
      && /opt/conda/bin/conda clean -afy; \
    elif [ "$BUILDPLATFORM" = "linux/arm64" ]; then \
      wget --quiet https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-aarch64.sh -O miniconda.sh -q \
      && mkdir -p /opt \
      && bash miniconda.sh -b -p /opt/conda \
      && rm miniconda.sh \
      && ln -s /opt/conda/etc/profile.d/conda.sh /etc/profile.d/conda.sh \
      && echo ". /opt/conda/etc/profile.d/conda.sh" >> ~/.bashrc \
      && echo "conda activate base" >> ~/.bashrc \
      && find /opt/conda/ -follow -type f -name '*.a' -delete \
      && find /opt/conda/ -follow -type f -name '*.js.map' -delete \
      && /opt/conda/bin/conda clean -afy; \
    fi

ENV PATH=/opt/conda/bin:$PATH

FROM nvidia/cuda:12.4.1-cudnn-devel-ubuntu22.04 AS cuda-base
FROM rocm/dev-ubuntu-22.04:6.2-complete AS rocm-base

FROM --platform=$BUILDPLATFORM ${src}-base AS image

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
       gcc-12 \
       g++-12 \
       pkg-config \
       python3 \
       python3-pip \
       python-is-python3 \
       python-dev-is-python3 \
       git \
       gdb \
       libopencv-dev \
       python3-opencv \
       unzip \
       libeigen3-dev \
       libomp-dev \
       libtbb-dev \
       libmosquitto-dev \
       libssl-dev \
       libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio libgstrtspserver-1.0-dev \
    && apt-get -y autoremove \
    && apt-get clean autoclean \
    && rm -fr /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*

ARG src

# Install libtorch and pytorch
COPY requirements*.txt /
RUN if [ "$src" = "cpu" ]; then \
      conda install -y pytorch-cpu \
      && conda install -y conda-forge::libtorch; \
    elif [ "$src" = "cuda" ]; then \
      python3 -m pip install --index-url https://download.pytorch.org/whl/cu121 torch torchvision torchaudio \
      && python3 -m pip install --index-url https://pypi.python.org/simple ultralytics opencv-python brotli \
      && wget --quiet -c https://download.pytorch.org/libtorch/cu121/libtorch-cxx11-abi-shared-with-deps-2.3.1%2Bcu121.zip --output-document libtorch.zip \
      && unzip libtorch.zip -d ~/src; \
    elif [ "$src" = "rocm" ]; then \
      python3 -m pip install --index-url https://download.pytorch.org/whl/rocm6.2 torch torchvision torchaudio \
      && python3 -m pip install --index-url https://pypi.python.org/simple ultralytics opencv-python brotli \
      && wget --quiet -c https://download.pytorch.org/libtorch/rocm6.2/libtorch-cxx11-abi-shared-with-deps-2.5.1%2Brocm6.2.zip --output-document libtorch.zip \
      && unzip libtorch.zip -d ~/src; \
    fi

ARG BUILDPLATFORM

# Install Pylon
RUN if [ -z "$LIVE" ]; then \
      if [ "$BUILDPLATFORM" = "linux/amd64" ]; then \
        wget --quiet https://www2.baslerweb.com/media/downloads/software/pylon_software/pylon-7.5.0.15658-linux-x86_64_debs.tar.gz -O pylon.tar.gz \
        && tar -xzf pylon.tar.gz \
        && dpkg -i pylon*.deb \
        && apt-get update \
        && DEBIAN_FRONTEND=noninteractive \
           apt-get -y --quiet --no-install-recommends install \
           net-tools \
           telnet \
           iputils-ping \
           iproute2 \
           build-essential \
           cmake \
           gcc-12 \
           g++-12 \
        && apt-get -y autoremove \
        && apt-get clean autoclean \
        && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*; \
      fi; \
    fi

# Install eCAL
RUN if [ "$BUILDPLATFORM" = "linux/amd64" ]; then \
      add-apt-repository ppa:ecal/ecal-latest \
      && apt-get update \
      && DEBIAN_FRONTEND=noninteractive \
         apt-get -y --quiet --no-install-recommends install \
         ecal \
         protobuf-compiler \
         libprotobuf-dev \
      && apt-get -y autoremove \
      && apt-get clean autoclean \
      && rm -rf /var/lib/apt/lists/{apt,dpkg,cache,log} /tmp/* /var/tmp/*; \
    fi

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
ENV CC gcc-12
ENV CXX g++-12

WORKDIR /src