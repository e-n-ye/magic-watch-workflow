ARG UBUNTU_IMAGE=ubuntu@sha256:561618e2c15bf2397621dd04f96926663a3b5616c189cf7e38db7e82f5c538ea

FROM ${UBUNTU_IMAGE} AS toolchain-builder

ARG CMAKE_VERSION=3.31.4
ARG CMAKE_SHA256=e8c096779b25acf006190b79b42a09f350e6d3c33ab7d395ed86a0871a9f8e96
ARG CMAKE_URL=https://cmake.org/files/v3.31/cmake-3.31.4-linux-x86_64.tar.gz
ARG NINJA_VERSION=1.13.2
ARG NINJA_SHA256=5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6
ARG NINJA_URL=https://github.com/ninja-build/ninja/releases/download/v1.13.2/ninja-linux.zip
ARG ARM_TOOLCHAIN_VERSION=13.3.rel1
ARG ARM_TOOLCHAIN_SHA256=95c011cee430e64dd6087c75c800f04b9c49832cc1000127a92a97f9c8d83af4
ARG ARM_TOOLCHAIN_URL=https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz
ARG CPPCHECK_VERSION=2.21.0
ARG CPPCHECK_SHA256=f028ff75ca5372738f3737c8b3e8611426a6526b6aea2ef01301ab0f5902f044
ARG CPPCHECK_URL=https://github.com/cppcheck-opensource/cppcheck/archive/refs/tags/2.21.0.tar.gz
ARG UBUNTU_MIRROR=http://mirrors.aliyun.com/ubuntu

ENV DEBIAN_FRONTEND=noninteractive

RUN sed -i \
        -e "s|http://archive.ubuntu.com/ubuntu|${UBUNTU_MIRROR}|g" \
        -e "s|http://security.ubuntu.com/ubuntu|${UBUNTU_MIRROR}|g" \
        /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get -o Acquire::Retries=5 update \
    && apt-get install --no-install-recommends --yes ca-certificates \
    && https_mirror="$(printf '%s' "${UBUNTU_MIRROR}" | sed 's|^http://|https://|')" \
    && sed -i "s|${UBUNTU_MIRROR}|${https_mirror}|g" /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get -o Acquire::Retries=5 update \
    && apt-get install --no-install-recommends --yes \
        build-essential \
        curl \
        python3 \
        unzip \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp/downloads

RUN curl --fail --ipv4 --location --retry 5 --retry-delay 2 \
        "${CMAKE_URL}" \
        --output cmake.tar.gz \
    && echo "${CMAKE_SHA256}  cmake.tar.gz" | sha256sum --check \
    && mkdir -p /opt/cmake \
    && tar --extract --gzip --file cmake.tar.gz --strip-components=1 --directory /opt/cmake \
    && rm cmake.tar.gz

RUN curl --fail --ipv4 --location --retry 5 --retry-delay 2 \
        "${NINJA_URL}" \
        --output ninja.zip \
    && echo "${NINJA_SHA256}  ninja.zip" | sha256sum --check \
    && unzip -p ninja.zip ninja > /usr/local/bin/ninja \
    && chmod 0755 /usr/local/bin/ninja \
    && rm ninja.zip

RUN curl --fail --ipv4 --location --retry 5 --retry-delay 2 \
        "${ARM_TOOLCHAIN_URL}" \
        --output arm-toolchain.tar.xz \
    && echo "${ARM_TOOLCHAIN_SHA256}  arm-toolchain.tar.xz" | sha256sum --check \
    && mkdir -p /opt/arm-none-eabi \
    && tar --extract --xz --file arm-toolchain.tar.xz --strip-components=1 --directory /opt/arm-none-eabi \
    && rm arm-toolchain.tar.xz

RUN curl --fail --ipv4 --location --retry 5 --retry-delay 2 \
        "${CPPCHECK_URL}" \
        --output cppcheck.tar.gz \
    && echo "${CPPCHECK_SHA256}  cppcheck.tar.gz" | sha256sum --check \
    && mkdir -p /tmp/cppcheck \
    && tar --extract --gzip --file cppcheck.tar.gz --strip-components=1 --directory /tmp/cppcheck \
    && install --directory /opt/cppcheck/bin /opt/cppcheck/share/cppcheck/cfg /opt/cppcheck/share/cppcheck/platforms \
    && make --directory /tmp/cppcheck --jobs="$(nproc)" MATCHCOMPILER=yes FILESDIR=/opt/cppcheck/share/cppcheck \
    && install --mode=0755 /tmp/cppcheck/cppcheck /opt/cppcheck/bin/cppcheck \
    && cp --archive /tmp/cppcheck/cfg/. /opt/cppcheck/share/cppcheck/cfg/ \
    && cp --archive /tmp/cppcheck/platforms/. /opt/cppcheck/share/cppcheck/platforms/ \
    && rm -rf /tmp/cppcheck cppcheck.tar.gz

FROM ${UBUNTU_IMAGE} AS f411-ci

ARG CMAKE_VERSION=3.31.4
ARG NINJA_VERSION=1.13.2
ARG ARM_TOOLCHAIN_VERSION=13.3.rel1
ARG CPPCHECK_VERSION=2.21.0
ARG UBUNTU_MIRROR=http://mirrors.aliyun.com/ubuntu

ENV DEBIAN_FRONTEND=noninteractive

RUN sed -i \
        -e "s|http://archive.ubuntu.com/ubuntu|${UBUNTU_MIRROR}|g" \
        -e "s|http://security.ubuntu.com/ubuntu|${UBUNTU_MIRROR}|g" \
        /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get -o Acquire::Retries=5 update \
    && apt-get install --no-install-recommends --yes ca-certificates \
    && https_mirror="$(printf '%s' "${UBUNTU_MIRROR}" | sed 's|^http://|https://|')" \
    && sed -i "s|${UBUNTU_MIRROR}|${https_mirror}|g" /etc/apt/sources.list.d/ubuntu.sources \
    && apt-get -o Acquire::Retries=5 update \
    && apt-get install --no-install-recommends --yes \
        git \
        libgcc-s1 \
        libgmp10 \
        libisl23 \
        libmpc3 \
        libmpfr6 \
        libncurses6 \
        libstdc++6 \
        zlib1g \
    && rm -rf /var/lib/apt/lists/*

COPY --from=toolchain-builder /opt/arm-none-eabi /opt/arm-none-eabi
COPY --from=toolchain-builder /opt/cmake /opt/cmake
COPY --from=toolchain-builder /opt/cppcheck /opt/cppcheck
COPY --from=toolchain-builder /usr/local/bin/ninja /usr/local/bin/ninja

ENV PATH=/opt/arm-none-eabi/bin:/opt/cmake/bin:/opt/cppcheck/bin:${PATH}
ENV ARM_GNU_TOOLCHAIN_VERSION=${ARM_TOOLCHAIN_VERSION}
ENV CMAKE_VERSION=${CMAKE_VERSION}
ENV NINJA_VERSION=${NINJA_VERSION}
ENV CPPCHECK_VERSION=${CPPCHECK_VERSION}

WORKDIR /workspace

LABEL org.opencontainers.image.title="Magic Watch F411 CI" \
      org.opencontainers.image.description="Pinned CMake, Ninja, Arm GNU Toolchain and Cppcheck environment for the F411 workflow"

CMD ["cmake", "--version"]
