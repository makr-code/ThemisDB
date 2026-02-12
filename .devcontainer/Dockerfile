# ThemisDB Dev Container
FROM mcr.microsoft.com/devcontainers/cpp:ubuntu-22.04

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        git \
        curl \
        zip \
        unzip \
        tar \
        pkg-config \
        ca-certificates \
        clang-format \
        clang-tidy \
        cppcheck \
        python3 \
        python3-pip \
        gdb \
        lldb \
    && apt-get autoremove -y && apt-get clean -y

RUN git clone https://github.com/Microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics \
    && ln -s /opt/vcpkg/vcpkg /usr/local/bin/vcpkg

ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

RUN pip3 install pre-commit

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=$USER_UID

WORKDIR /workspace

CMD ["/bin/bash"]
