# syntax=docker/dockerfile:1

FROM centos:7 AS builder-base
ENV LANG en_US.UTF-8
ENV LIBRARY_PATH /usr/local/lib64:/usr/local/lib:/lib64:/lib:/usr/lib64:/usr/lib
ENV LD_LIBRARY_PATH $LIBRARY_PATH
ENV PKG_CONFIG_PATH /usr/local/lib64/pkgconfig:/usr/local/lib/pkgconfig:/usr/local/share/pkgconfig

RUN yum -y install https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
RUN yum -y install https://packages.endpointdev.com/rhel/7/os/x86_64/endpoint-repo.x86_64.rpm

#COPY endpoint-repo.x86_64.rpm /tmp && yum -y install /tmp/endpoint-repo.x86_64.rpm
RUN yum -y install centos-release-scl \
	&& yum -y install git rh-python38-python-pip rh-python38-python-devel \
		autoconf automake libtool patch gperf flex \
		fontconfig-devel freetype-devel libX11-devel at-spi2-core-devel alsa-lib-devel \
		pulseaudio-libs-devel mesa-libGL-devel mesa-libEGL-devel mesa-libgbm-devel \
		libdrm-devel vulkan-devel boost169-devel fmt-devel \
		gtk3-devel perl-XML-Parser pkgconfig bison yasm file which xorg-x11-util-macros \
		devtoolset-10-make devtoolset-10-gcc devtoolset-10-gcc-c++ \
		devtoolset-10-binutils llvm-toolset-7.0 llvm-toolset-7.0-clang-devel \
		llvm-toolset-7.0-llvm-devel \
	&& yum clean all

# Fix a bug with argument naming in CentOS 7 glibc
RUN sed -i 's/char \*__block/char */' /usr/include/unistd.h

SHELL [ "bash", "-c", ". /opt/rh/rh-python38/enable; . /opt/rh/devtoolset-10/enable; exec bash -c \"$@\"", "-s"]

WORKDIR /usr/src/Libraries

RUN python3 -m pip install meson ninja

RUN mkdir /opt/cmake \
	&& curl -sSLo cmake-3.26.3-Linux-x86_64.sh https://github.com/Kitware/CMake/releases/download/v3.26.3/cmake-3.26.3-Linux-x86_64.sh \
	&& sh cmake-3.26.3-Linux-x86_64.sh --prefix=/opt/cmake --skip-license \
	&& ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake \
	&& rm cmake-3.26.3-Linux-x86_64.sh

FROM builder-base AS builder
ENV AR gcc-ar
ENV RANLIB gcc-ranlib
ENV NM gcc-nm
ENV CFLAGS -g -O3 -flto=auto -ffat-lto-objects -pipe -fPIC -fstack-protector-all -fstack-clash-protection -DNDEBUG -D_FORTIFY_SOURCE=2 -D_GLIBCXX_ASSERTIONS
ENV CXXFLAGS $CFLAGS

FROM builder AS patches
RUN git clone https://github.com/desktop-app/patches.git \
	&& cd patches \
	&& git checkout 1d72fdb335 \
	&& rm -rf .git

FROM builder AS nasm
RUN git clone -b nasm-2.15.05 --depth=1 https://github.com/netwide-assembler/nasm.git \
	&& cd nasm \
	&& ./autogen.sh \
	&& ./configure \
	&& make -j$(nproc) \
	&& mkdir -p "/usr/src/Libraries/nasm-cache/usr/local/bin" \
	&& install -c nasm "/usr/src/Libraries/nasm-cache/usr/local/bin/nasm" \
	&& cd .. \
	&& rm -rf nasm

FROM builder AS libffi
RUN git clone -b v3.4.2 --depth=1 https://github.com/libffi/libffi.git \
	&& cd libffi \
	&& ./autogen.sh \
	&& ./configure --enable-static --disable-docs \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libffi-cache" install \
	&& cd .. \
	&& rm -rf libffi

FROM builder AS zlib
RUN git clone -b v1.2.11 --depth=1 https://github.com/madler/zlib.git \
	&& cd zlib \
	&& ./configure \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/zlib-cache" install \
	&& cd .. \
	&& rm -rf zlib

FROM builder AS xz
RUN git clone -b v5.2.5 https://git.tukaani.org/xz.git \
	&& cd xz \
	&& cmake -GNinja -B build . -DCMAKE_BUILD_TYPE=None \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/xz-cache" cmake --install build \
	&& cd .. \
	&& rm -rf xz

FROM builder AS protobuf
RUN git clone --recursive -b v21.9 https://github.com/protocolbuffers/protobuf \
    && cd protobuf \
    && git clone https://github.com/abseil/abseil-cpp third_party/abseil-cpp \
    && cd third_party/abseil-cpp \
    && git checkout 273292d1cf \
    && cd ../.. \
    && mkdir build \
    && cd build \
    && cmake -GNinja .. \
    	-DCMAKE_BUILD_TYPE=None \
		-DCMAKE_C_FLAGS="$CFLAGS" \
		-DCMAKE_CXX_FLAGS="$CXXFLAGS" \
        -Dprotobuf_BUILD_TESTS=OFF \
        -Dprotobuf_BUILD_PROTOBUF_BINARIES=ON \
        -Dprotobuf_BUILD_LIBPROTOC=ON \
        -Dprotobuf_WITH_ZLIB_DEFAULT=OFF \
        -Dprotobuf_DEBUG_POSTFIX="" \
    && cmake --build . --parallel \
    && cd .. \
    && rm -rf .git

FROM patches AS libproxy
RUN git clone -b 0.4.17 --depth=1 https://github.com/libproxy/libproxy.git \
	&& cd libproxy \
	&& git apply ../patches/libproxy.patch \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DWITH_DBUS=OFF \
		-DWITH_NM=OFF \
		-DWITH_NMold=OFF \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/libproxy-cache" cmake --install build \
	&& cd .. \
	&& rm -rf libproxy

FROM builder AS lcms2
RUN git clone -b lcms2.13.1 --depth=1 https://github.com/mm2/Little-CMS.git \
	&& cd Little-CMS \
	&& meson build \
		--buildtype=plain \
		--default-library=both \
	&& meson compile -C build \
	&& DESTDIR="/usr/src/Libraries/lcms2-cache" meson install -C build \
	&& cd .. \
	&& rm -rf Little-CMS

FROM builder AS brotli
RUN git clone -b v1.0.9 --depth=1 https://github.com/google/brotli.git \
	&& cd brotli \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DBROTLI_DISABLE_TESTS=ON \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/brotli-cache" cmake --install build \
	&& cd .. \
	&& rm -rf brotli

FROM builder AS highway
RUN git clone -b 1.0.3 --depth=1 https://github.com/google/highway.git \
	&& cd highway \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DBUILD_TESTING=OFF \
		-DHWY_ENABLE_CONTRIB=OFF \
		-DHWY_ENABLE_EXAMPLES=OFF \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/highway-cache" cmake --install build \
	&& cd .. \
	&& rm -rf highway

FROM builder AS mozjpeg
RUN git clone -b v4.0.3 --depth=1 https://github.com/mozilla/mozjpeg.git \
	&& cd mozjpeg \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DWITH_JPEG8=ON \
		-DPNG_SUPPORTED=OFF \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/mozjpeg-cache" cmake --install build \
	&& cd .. \
	&& rm -rf mozjpeg

FROM builder AS opus
RUN git clone -b v1.3.1 --depth=1 https://github.com/xiph/opus.git \
	&& cd opus \
	&& ./autogen.sh \
	&& ./configure \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/opus-cache" install \
	&& cd .. \
	&& rm -rf opus

FROM builder AS dav1d
COPY --from=nasm /usr/src/Libraries/nasm-cache /

RUN git clone -b 1.0.0 --depth=1 https://github.com/videolan/dav1d.git \
	&& cd dav1d \
	&& meson build \
		--buildtype=plain \
		--default-library=both \
		-Denable_tools=false \
		-Denable_tests=false \
	&& meson compile -C build \
	&& DESTDIR="/usr/src/Libraries/dav1d-cache" meson install -C build \
	&& cd .. \
	&& rm -rf dav1d

FROM builder AS libde265
RUN git clone -b v1.0.11 --depth=1 https://github.com/strukturag/libde265.git \
	&& cd libde265 \
	&& cmake -GNinja . \
		-DCMAKE_BUILD_TYPE=None \
		-DBUILD_SHARED_LIBS=OFF \
		-DENABLE_DECODER=OFF \
		-DENABLE_SDL=OFF \
	&& cmake --build . --parallel \
	&& DESTDIR="/usr/src/Libraries/libde265-cache" cmake --install . \
	&& cd .. \
	&& rm -rf libde265

FROM builder AS libvpx
RUN git clone -b v1.11.0 --depth=1 https://github.com/webmproject/libvpx.git \
	&& cd libvpx \
	&& CFLAGS="$CFLAGS -fno-lto" CXXFLAGS="$CXXFLAGS -fno-lto" ./configure \
		--disable-examples \
		--disable-unit-tests \
		--disable-tools \
		--disable-docs \
		--enable-vp8 \
		--enable-vp9 \
		--enable-webm-io \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libvpx-cache" install \
	&& cd .. \
	&& rm -rf libvpx

FROM builder AS libavif
COPY --from=dav1d /usr/src/Libraries/dav1d-cache /

RUN git clone -b v0.11.1 --depth=1 https://github.com/AOMediaCodec/libavif.git \
	&& cd libavif \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DBUILD_SHARED_LIBS=OFF \
		-DAVIF_CODEC_DAV1D=ON \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/libavif-cache" cmake --install build \
	&& cd .. \
	&& rm -rf libavif

FROM builder AS libheif
COPY --from=libde265 /usr/src/Libraries/libde265-cache /

RUN git clone -b v1.15.1 --depth=1 https://github.com/strukturag/libheif.git \
	&& cd libheif \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DBUILD_SHARED_LIBS=OFF \
		-DENABLE_PLUGIN_LOADING=OFF \
		-DWITH_X265=OFF \
		-DWITH_AOM_DECODER=OFF \
		-DWITH_AOM_ENCODER=OFF \
		-DWITH_RAV1E=OFF \
		-DWITH_RAV1E_PLUGIN=OFF \
		-DWITH_SvtEnc=OFF \
		-DWITH_SvtEnc_PLUGIN=OFF \
		-DWITH_DAV1D=OFF \
		-DWITH_EXAMPLES=OFF \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/libheif-cache" cmake --install build \
	&& cd .. \
	&& rm -rf libheif

FROM builder AS libjxl
COPY --from=lcms2 /usr/src/Libraries/lcms2-cache /
COPY --from=brotli /usr/src/Libraries/brotli-cache /
COPY --from=highway /usr/src/Libraries/highway-cache /

RUN git clone -b v0.8.1 --depth=1 https://github.com/libjxl/libjxl.git \
	&& cd libjxl \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTING=OFF \
		-DJPEGXL_ENABLE_DEVTOOLS=OFF \
		-DJPEGXL_ENABLE_TOOLS=OFF \
		-DJPEGXL_ENABLE_JPEGLI_LIBJPEG=OFF \
		-DJPEGXL_ENABLE_DOXYGEN=OFF \
		-DJPEGXL_ENABLE_MANPAGES=OFF \
		-DJPEGXL_ENABLE_BENCHMARK=OFF \
		-DJPEGXL_ENABLE_EXAMPLES=OFF \
		-DJPEGXL_ENABLE_JNI=OFF \
		-DJPEGXL_ENABLE_SJPEG=OFF \
		-DJPEGXL_ENABLE_OPENEXR=OFF \
		-DJPEGXL_ENABLE_SKCMS=OFF \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/libjxl-cache" cmake --install build \
	&& cd .. \
	&& rm -rf libjxl

FROM builder AS rnnoise
RUN git clone -b master --depth=1 https://github.com/desktop-app/rnnoise \
	&& cd rnnoise \
	&& cmake -GNinja -B build . -DCMAKE_BUILD_TYPE=None \
	&& cmake --build build --parallel \
	&& mkdir -p "/usr/src/Libraries/rnnoise-cache/usr/local/include" \
	&& cp "include/rnnoise.h" "/usr/src/Libraries/rnnoise-cache/usr/local/include/" \
	&& mkdir -p "/usr/src/Libraries/rnnoise-cache/usr/local/lib" \
	&& cp "build/librnnoise.a" "/usr/src/Libraries/rnnoise-cache/usr/local/lib/" \
	&& cd .. \
	&& rm -rf rnnoise

FROM builder AS xcb-proto
RUN git clone -b xcb-proto-1.14.1 --depth=1 https://github.com/gitlab-freedesktop-mirrors/xcbproto.git \
	&& cd xcbproto \
	&& ./autogen.sh \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-proto-cache" install \
	&& cd .. \
	&& rm -rf xcbproto

FROM builder AS xcb
COPY --from=xcb-proto /usr/src/Libraries/xcb-proto-cache /

RUN git clone -b libxcb-1.14 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxcb.git \
	&& cd libxcb \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-cache" install \
	&& cd .. \
	&& rm -rf libxcb

FROM builder AS xcb-wm
RUN git clone -b 0.4.1 --depth=1 --recursive https://github.com/gitlab-freedesktop-mirrors/libxcb-wm.git \
	&& cd libxcb-wm \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-wm-cache" install \
	&& cd .. \
	&& rm -rf libxcb-wm

FROM builder AS xcb-util
RUN git clone -b 0.4.0 --depth=1 --recursive https://github.com/gitlab-freedesktop-mirrors/libxcb-util.git \
	&& cd libxcb-util \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-util-cache" install \
	&& cd .. \
	&& rm -rf libxcb-util

FROM builder AS xcb-image
COPY --from=xcb-util /usr/src/Libraries/xcb-util-cache /

RUN git clone -b 0.4.0 --depth=1 --recursive https://github.com/gitlab-freedesktop-mirrors/libxcb-image.git \
	&& cd libxcb-image \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-image-cache" install \
	&& cd .. \
	&& rm -rf libxcb-image

FROM builder AS xcb-keysyms
RUN git clone -b 0.4.0 --depth=1 --recursive https://github.com/gitlab-freedesktop-mirrors/libxcb-keysyms.git \
	&& cd libxcb-keysyms \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-keysyms-cache" install \
	&& cd .. \
	&& rm -rf libxcb-keysyms

FROM builder AS xcb-render-util
RUN git clone -b 0.3.9 --depth=1 --recursive https://github.com/gitlab-freedesktop-mirrors/libxcb-render-util.git \
	&& cd libxcb-render-util \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-render-util-cache" install \
	&& cd .. \
	&& rm -rf libxcb-render-util

FROM builder AS xcb-cursor
COPY --from=xcb-util /usr/src/Libraries/xcb-util-cache /
COPY --from=xcb-image /usr/src/Libraries/xcb-image-cache /
COPY --from=xcb-render-util /usr/src/Libraries/xcb-render-util-cache /

RUN git clone -b xcb-util-cursor-0.1.4 --depth=1 --recursive https://github.com/gitlab-freedesktop-mirrors/libxcb-cursor.git \
	&& cd libxcb-cursor \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/xcb-cursor-cache" install \
	&& cd .. \
	&& rm -rf libxcb-cursor

FROM builder AS libXext
RUN git clone -b libXext-1.3.4 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxext.git \
	&& cd libxext \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libXext-cache" install \
	&& cd .. \
	&& rm -rf libxext

FROM builder AS libXtst
RUN git clone -b libXtst-1.2.3 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxtst.git \
	&& cd libxtst \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libXtst-cache" install \
	&& cd .. \
	&& rm -rf libxtst

FROM builder AS libXfixes
RUN git clone -b libXfixes-5.0.3 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxfixes.git \
	&& cd libxfixes \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libXfixes-cache" install \
	&& cd .. \
	&& rm -rf libxfixes

FROM builder AS libXrandr
RUN git clone -b libXrandr-1.5.2 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxrandr.git \
	&& cd libxrandr \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libXrandr-cache" install \
	&& cd .. \
	&& rm -rf libxrandr

FROM builder AS libXrender
RUN git clone -b libXrender-0.9.10 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxrender.git \
	&& cd libxrender \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libXrender-cache" install \
	&& cd .. \
	&& rm -rf libxrender

FROM builder AS libXdamage
RUN git clone -b libXdamage-1.1.5 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxdamage.git \
	&& cd libxdamage \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libXdamage-cache" install \
	&& cd .. \
	&& rm -rf libxdamage

FROM builder AS libXcomposite
RUN git clone -b libXcomposite-0.4.5 --depth=1 https://github.com/gitlab-freedesktop-mirrors/libxcomposite.git \
	&& cd libxcomposite \
	&& ./autogen.sh --enable-static \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/libXcomposite-cache" install \
	&& cd .. \
	&& rm -rf libxcomposite

FROM builder AS wayland
COPY --from=libffi /usr/src/Libraries/libffi-cache /

RUN git clone -b 1.21.0 --depth=1 https://github.com/gitlab-freedesktop-mirrors/wayland.git \
	&& cd wayland \
	&& meson build \
		--buildtype=plain \
		--default-library=both \
		-Dtests=false \
		-Ddocumentation=false \
		-Ddtd_validation=false \
		-Dicon_directory=/usr/share/icons \
	&& meson compile -C build \
	&& DESTDIR="/usr/src/Libraries/wayland-cache" meson install -C build \
	&& cd .. \
	&& rm -rf wayland

FROM builder AS nv-codec-headers
RUN git clone -b n11.1.5.1 --depth=1 https://github.com/FFmpeg/nv-codec-headers.git \
	&& DESTDIR="/usr/src/Libraries/nv-codec-headers-cache" make -C nv-codec-headers install \
	&& rm -rf nv-codec-headers

FROM builder AS ffmpeg
COPY --from=opus /usr/src/Libraries/opus-cache /
COPY --from=dav1d /usr/src/Libraries/dav1d-cache /
COPY --from=libvpx /usr/src/Libraries/libvpx-cache /
COPY --from=nv-codec-headers /usr/src/Libraries/nv-codec-headers-cache /

RUN git init ffmpeg \
	&& cd ffmpeg \
	&& git remote add origin https://github.com/FFmpeg/FFmpeg.git \
	&& git fetch --depth=1 origin 7268323193d55365f914de39fadd5dbdb1f68976 \
	&& git reset --hard FETCH_HEAD \
	&& ./configure \
		--extra-cflags="-DCONFIG_SAFE_BITSTREAM_READER=1" \
		--extra-cxxflags="-DCONFIG_SAFE_BITSTREAM_READER=1" \
		--disable-debug \
		--disable-optimizations \
		--disable-inline-asm \
		--disable-programs \
		--disable-doc \
		--disable-network \
		--disable-autodetect \
		--disable-everything \
		--enable-libdav1d \
		--enable-libopus \
		--enable-libvpx \
		--enable-ffnvcodec \
		--enable-nvdec \
		--enable-cuvid \
		--enable-protocol=file \
		--enable-hwaccel=av1_nvdec \
		--enable-hwaccel=h264_nvdec \
		--enable-hwaccel=hevc_nvdec \
		--enable-hwaccel=mpeg2_nvdec \
		--enable-hwaccel=mpeg4_nvdec \
		--enable-hwaccel=vp8_nvdec \
		--enable-decoder=aac \
		--enable-decoder=aac_fixed \
		--enable-decoder=aac_latm \
		--enable-decoder=aasc \
		--enable-decoder=ac3 \
		--enable-decoder=alac \
		--enable-decoder=av1 \
		--enable-decoder=av1_cuvid \
		--enable-decoder=eac3 \
		--enable-decoder=flac \
		--enable-decoder=gif \
		--enable-decoder=h264 \
		--enable-decoder=hevc \
		--enable-decoder=libdav1d \
		--enable-decoder=libvpx_vp8 \
		--enable-decoder=libvpx_vp9 \
		--enable-decoder=mp1 \
		--enable-decoder=mp1float \
		--enable-decoder=mp2 \
		--enable-decoder=mp2float \
		--enable-decoder=mp3 \
		--enable-decoder=mp3adu \
		--enable-decoder=mp3adufloat \
		--enable-decoder=mp3float \
		--enable-decoder=mp3on4 \
		--enable-decoder=mp3on4float \
		--enable-decoder=mpeg4 \
		--enable-decoder=msmpeg4v2 \
		--enable-decoder=msmpeg4v3 \
		--enable-decoder=opus \
		--enable-decoder=pcm_alaw \
		--enable-decoder=pcm_f32be \
		--enable-decoder=pcm_f32le \
		--enable-decoder=pcm_f64be \
		--enable-decoder=pcm_f64le \
		--enable-decoder=pcm_lxf \
		--enable-decoder=pcm_mulaw \
		--enable-decoder=pcm_s16be \
		--enable-decoder=pcm_s16be_planar \
		--enable-decoder=pcm_s16le \
		--enable-decoder=pcm_s16le_planar \
		--enable-decoder=pcm_s24be \
		--enable-decoder=pcm_s24daud \
		--enable-decoder=pcm_s24le \
		--enable-decoder=pcm_s24le_planar \
		--enable-decoder=pcm_s32be \
		--enable-decoder=pcm_s32le \
		--enable-decoder=pcm_s32le_planar \
		--enable-decoder=pcm_s64be \
		--enable-decoder=pcm_s64le \
		--enable-decoder=pcm_s8 \
		--enable-decoder=pcm_s8_planar \
		--enable-decoder=pcm_u16be \
		--enable-decoder=pcm_u16le \
		--enable-decoder=pcm_u24be \
		--enable-decoder=pcm_u24le \
		--enable-decoder=pcm_u32be \
		--enable-decoder=pcm_u32le \
		--enable-decoder=pcm_u8 \
		--enable-decoder=pcm_zork \
		--enable-decoder=vorbis \
		--enable-decoder=vp8 \
		--enable-decoder=wavpack \
		--enable-decoder=wmalossless \
		--enable-decoder=wmapro \
		--enable-decoder=wmav1 \
		--enable-decoder=wmav2 \
		--enable-decoder=wmavoice \
		--enable-encoder=libopus \
		--enable-filter=atempo \
		--enable-parser=aac \
		--enable-parser=aac_latm \
		--enable-parser=flac \
		--enable-parser=h264 \
		--enable-parser=hevc \
		--enable-parser=mpeg4video \
		--enable-parser=mpegaudio \
		--enable-parser=opus \
		--enable-parser=vorbis \
		--enable-demuxer=aac \
		--enable-demuxer=flac \
		--enable-demuxer=gif \
		--enable-demuxer=h264 \
		--enable-demuxer=hevc \
		--enable-demuxer=matroska \
		--enable-demuxer=m4v \
		--enable-demuxer=mov \
		--enable-demuxer=mp3 \
		--enable-demuxer=ogg \
		--enable-demuxer=wav \
		--enable-muxer=ogg \
		--enable-muxer=opus \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/ffmpeg-cache" install \
	&& cd .. \
	&& rm -rf ffmpeg

FROM builder AS pipewire
RUN git clone -b 0.3.25 --depth=1 https://github.com/PipeWire/pipewire.git \
	&& cd pipewire \
	&& meson build \
		--buildtype=plain \
		-Dtests=disabled \
		-Dexamples=disabled \
		-Dspa-plugins=disabled \
	&& meson compile -C build \
	&& DESTDIR="/usr/src/Libraries/pipewire-cache" meson install -C build \
	&& cd .. \
	&& rm -rf pipewire

FROM builder AS openal
COPY --from=pipewire /usr/src/Libraries/pipewire-cache /

RUN git clone -b 1.23.0 --depth=1 https://github.com/kcat/openal-soft.git \
	&& cd openal-soft \
	&& cmake -GNinja -B build . \
		-DCMAKE_BUILD_TYPE=None \
		-DLIBTYPE:STRING=STATIC \
		-DALSOFT_EXAMPLES=OFF \
		-DALSOFT_UTILS=OFF \
		-DALSOFT_INSTALL_CONFIG=OFF \
	&& cmake --build build --parallel \
	&& DESTDIR="/usr/src/Libraries/openal-cache" cmake --install build \
	&& cd .. \
	&& rm -rf openal-soft

FROM builder AS openssl
ENV opensslDir openssl_1_1_1
RUN git clone -b OpenSSL_1_1_1-stable --depth=1 https://github.com/openssl/openssl.git $opensslDir \
	&& cd $opensslDir \
	&& ./config \
		--prefix="/usr/local/desktop-app/openssl-1.1.1" \
		--openssldir=/etc/ssl \
		no-tests \
		no-dso \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/openssl-cache" install_sw \
	&& cd .. \
	&& rm -rf $opensslDir

FROM builder AS xkbcommon
COPY --from=xcb /usr/src/Libraries/xcb-cache /

RUN git clone -b xkbcommon-1.3.1 --depth=1 https://github.com/xkbcommon/libxkbcommon.git \
	&& cd libxkbcommon \
	&& meson build \
		--buildtype=plain \
		--default-library=both \
		-Denable-docs=false \
		-Denable-wayland=false \
		-Denable-xkbregistry=false \
		-Dxkb-config-root=/usr/share/X11/xkb \
		-Dxkb-config-extra-path=/etc/xkb \
		-Dx-locale-root=/usr/share/X11/locale \
	&& meson compile -C build \
	&& DESTDIR="/usr/src/Libraries/xkbcommon-cache" meson install -C build \
	&& cd .. \
	&& rm -rf libxkbcommon

FROM patches AS glibmm
COPY --from=libffi /usr/src/Libraries/libffi-cache /

RUN git clone -b 2.76.0 --depth=1 https://github.com/GNOME/glibmm.git \
	&& cd glibmm \
	&& git apply ../patches/glibmm.patch \
	&& CFLAGS="$CFLAGS -flto=auto -ffat-lto-objects" CXXFLAGS="$CXXFLAGS -flto=auto -ffat-lto-objects" meson build \
		--buildtype=plain \
		--default-library=both \
		-Dbuild-documentation=false \
		-Dbuild-examples=false \
		-Dsigc++-3.0:build-documentation=false \
		-Dsigc++-3.0:build-examples=false \
		-Dsigc++-3.0:validation=false \
		-Dglib:tests=false \
		-Dmm-common:use-network=true \
	&& meson compile -C build \
	&& DESTDIR="/usr/src/Libraries/glibmm-cache" meson install -C build \
	&& cd .. \
	&& rm -rf glibmm

FROM builder AS gobject-introspection
COPY --from=libffi /usr/src/Libraries/libffi-cache /
COPY --from=glibmm /usr/src/Libraries/glibmm-cache /

RUN git clone -b 1.76.1 --depth=1 https://github.com/GNOME/gobject-introspection.git \
	&& cd gobject-introspection \
	&& meson build --buildtype=plain \
	&& meson compile -C build \
	&& DESTDIR="/usr/src/Libraries/gobject-introspection-cache" meson install -C build \
	&& cd .. \
	&& rm -rf gobject-introspection

FROM patches AS qt
COPY --from=libffi /usr/src/Libraries/libffi-cache /
COPY --from=zlib /usr/src/Libraries/zlib-cache /
COPY --from=libproxy /usr/src/Libraries/libproxy-cache /
COPY --from=lcms2 /usr/src/Libraries/lcms2-cache /
COPY --from=mozjpeg /usr/src/Libraries/mozjpeg-cache /
COPY --from=xcb /usr/src/Libraries/xcb-cache /
COPY --from=xcb-wm /usr/src/Libraries/xcb-wm-cache /
COPY --from=xcb-util /usr/src/Libraries/xcb-util-cache /
COPY --from=xcb-image /usr/src/Libraries/xcb-image-cache /
COPY --from=xcb-keysyms /usr/src/Libraries/xcb-keysyms-cache /
COPY --from=xcb-render-util /usr/src/Libraries/xcb-render-util-cache /
COPY --from=xcb-cursor /usr/src/Libraries/xcb-cursor-cache /
COPY --from=wayland /usr/src/Libraries/wayland-cache /
COPY --from=openssl /usr/src/Libraries/openssl-cache /
COPY --from=xkbcommon /usr/src/Libraries/xkbcommon-cache /

ENV OPENSSL_ROOT_DIR /usr/local/desktop-app/openssl-1.1.1

RUN git clone -b v6.5.1 --depth=1 https://code.qt.io/qt/qt5.git qt_6.5.1 \
	&& cd qt_6.5.1 \
	&& git submodule update --init --recursive --depth=1 qtbase qtdeclarative qtwayland qtimageformats qtsvg qtshadertools \
	&& cd qtbase \
	&& find ../../patches/qtbase_6.5.1 -type f -print0 | sort -z | xargs -r0 git apply \
	&& cd ../qtdeclarative \
	&& sed -i '/add_subdirectory(quickcontrols)/d' src/CMakeLists.txt \
	&& sed -i '/add_subdirectory(quickdialogs)/d' src/CMakeLists.txt \
	&& sed -i '/add_subdirectory(quicknativestyle)/d' src/CMakeLists.txt \
	&& cd .. \
	&& ./configure -prefix "/usr/local/desktop-app/Qt-6.5.1" \
		CMAKE_BUILD_TYPE=None \
		-opensource \
		-confirm-license \
		-libproxy \
		-qt-libpng \
		-qt-harfbuzz \
		-qt-pcre \
		-no-icu \
		-no-feature-xcb-sm \
		-no-feature-egl-extension-platform-wayland \
		-static \
		-dbus-runtime \
		-openssl-linked \
		-nomake examples \
		-nomake tests \
	&& cmake --build . --parallel \
	&& DESTDIR="/usr/src/Libraries/qt-cache" cmake --install . \
	&& cd .. \
	&& rm -rf qt_6.5.1

FROM patches AS breakpad
RUN git clone https://chromium.googlesource.com/breakpad/breakpad.git \
	&& cd breakpad \
	&& git checkout dfcb7b6799 \
	&& git apply ../patches/breakpad.diff \
	&& git clone https://chromium.googlesource.com/linux-syscall-support.git src/third_party/lss \
	&& cd src/third_party/lss \
	&& git checkout e1e7b0ad8e \
	&& cd ../../.. \
	&& env -u CFLAGS -u CXXFLAGS ./configure \
	&& make -j$(nproc) \
	&& make DESTDIR="/usr/src/Libraries/breakpad-cache" install \
	&& cd .. \
	&& rm -rf breakpad

FROM builder AS webrtc
COPY --from=mozjpeg /usr/src/Libraries/mozjpeg-cache /
COPY --from=opus /usr/src/Libraries/opus-cache /
COPY --from=libvpx /usr/src/Libraries/libvpx-cache /
COPY --from=ffmpeg /usr/src/Libraries/ffmpeg-cache /
COPY --from=openssl /usr/src/Libraries/openssl-cache /
COPY --from=libXtst /usr/src/Libraries/libXtst-cache /
COPY --from=pipewire /usr/src/Libraries/pipewire-cache /

# Shallow clone on a specific commit.
RUN git init tg_owt \
	&& cd tg_owt \
	&& git remote add origin https://github.com/desktop-app/tg_owt.git \
	&& git fetch --depth=1 origin dcb5069ff76bd293e86928804208737e6cee2ccc \
	&& git reset --hard FETCH_HEAD \
	&& git submodule update --init --recursive --depth=1 \
	&& rm -rf .git \
	&& env -u CFLAGS -u CXXFLAGS cmake -G"Ninja Multi-Config" -B out . \
		-DCMAKE_C_FLAGS_RELEASE="$CFLAGS" \
		-DCMAKE_C_FLAGS_DEBUG="-g -pipe -fPIC -fstack-protector-all -fstack-clash-protection -D_GLIBCXX_ASSERTIONS" \
		-DCMAKE_CXX_FLAGS_RELEASE="$CXXFLAGS" \
		-DCMAKE_CXX_FLAGS_DEBUG="-g -pipe -fPIC -fstack-protector-all -fstack-clash-protection -D_GLIBCXX_ASSERTIONS" \
		-DTG_OWT_SPECIAL_TARGET=linux \
		-DTG_OWT_LIBJPEG_INCLUDE_PATH=/usr/local/include \
		-DTG_OWT_OPENSSL_INCLUDE_PATH=/usr/local/desktop-app/openssl-1.1.1/include \
		-DTG_OWT_OPUS_INCLUDE_PATH=/usr/local/include/opus \
		-DTG_OWT_LIBVPX_INCLUDE_PATH=/usr/local/include \
		-DTG_OWT_FFMPEG_INCLUDE_PATH=/usr/local/include

WORKDIR tg_owt

FROM webrtc AS webrtc_release
RUN cmake --build out --config Release --parallel \
	&& find out -mindepth 1 -maxdepth 1 ! -name Release -exec rm -rf {} \;

FROM webrtc AS webrtc_debug
RUN cmake --build out --config Debug --parallel \
	&& find out -mindepth 1 -maxdepth 1 ! -name Debug -exec rm -rf {} \;

FROM builder-base
COPY --from=libffi /usr/src/Libraries/libffi-cache /
COPY --from=zlib /usr/src/Libraries/zlib-cache /
COPY --from=xz /usr/src/Libraries/xz-cache /
COPY --from=protobuf /usr/src/Libraries/protobuf protobuf
COPY --from=libproxy /usr/src/Libraries/libproxy-cache /
COPY --from=lcms2 /usr/src/Libraries/lcms2-cache /
COPY --from=brotli /usr/src/Libraries/brotli-cache /
COPY --from=highway /usr/src/Libraries/highway-cache /
COPY --from=mozjpeg /usr/src/Libraries/mozjpeg-cache /
COPY --from=opus /usr/src/Libraries/opus-cache /
COPY --from=dav1d /usr/src/Libraries/dav1d-cache /
COPY --from=libde265 /usr/src/Libraries/libde265-cache /
COPY --from=libvpx /usr/src/Libraries/libvpx-cache /
COPY --from=libavif /usr/src/Libraries/libavif-cache /
COPY --from=libheif /usr/src/Libraries/libheif-cache /
COPY --from=libjxl /usr/src/Libraries/libjxl-cache /
COPY --from=rnnoise /usr/src/Libraries/rnnoise-cache /
COPY --from=xcb /usr/src/Libraries/xcb-cache /
COPY --from=xcb-wm /usr/src/Libraries/xcb-wm-cache /
COPY --from=xcb-util /usr/src/Libraries/xcb-util-cache /
COPY --from=xcb-image /usr/src/Libraries/xcb-image-cache /
COPY --from=xcb-keysyms /usr/src/Libraries/xcb-keysyms-cache /
COPY --from=xcb-render-util /usr/src/Libraries/xcb-render-util-cache /
COPY --from=xcb-cursor /usr/src/Libraries/xcb-cursor-cache /
COPY --from=libXext /usr/src/Libraries/libXext-cache /
COPY --from=libXfixes /usr/src/Libraries/libXfixes-cache /
COPY --from=libXtst /usr/src/Libraries/libXtst-cache /
COPY --from=libXrandr /usr/src/Libraries/libXrandr-cache /
COPY --from=libXrender /usr/src/Libraries/libXrender-cache /
COPY --from=libXdamage /usr/src/Libraries/libXdamage-cache /
COPY --from=libXcomposite /usr/src/Libraries/libXcomposite-cache /
COPY --from=wayland /usr/src/Libraries/wayland-cache /
COPY --from=ffmpeg /usr/src/Libraries/ffmpeg-cache /
COPY --from=openal /usr/src/Libraries/openal-cache /
COPY --from=openssl /usr/src/Libraries/openssl-cache /
COPY --from=xkbcommon /usr/src/Libraries/xkbcommon-cache /
COPY --from=glibmm /usr/src/Libraries/glibmm-cache /
COPY --from=gobject-introspection /usr/src/Libraries/gobject-introspection-cache /
COPY --from=qt /usr/src/Libraries/qt-cache /
COPY --from=breakpad /usr/src/Libraries/breakpad-cache /
COPY --from=webrtc /usr/src/Libraries/tg_owt tg_owt
COPY --from=webrtc_release /usr/src/Libraries/tg_owt/out/Release tg_owt/out/Release
COPY --from=webrtc_debug /usr/src/Libraries/tg_owt/out/Debug tg_owt/out/Debug

WORKDIR ../tdesktop
ENV QT 6.5.1
ENV OPENSSL_ROOT_DIR /usr/local/desktop-app/openssl-1.1.1
ENV BOOST_INCLUDEDIR /usr/include/boost169
ENV BOOST_LIBRARYDIR /usr/lib64/boost169

VOLUME [ "/usr/src/tdesktop" ]
ENTRYPOINT [ "scl", "enable", "rh-python38", "--", "scl", "enable", "llvm-toolset-7.0", "--", "scl", "enable", "devtoolset-10", "--" ]
CMD [ "/usr/src/tdesktop/Telegram/build/docker/centos_env/build.sh" ]
