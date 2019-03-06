FROM ubuntu:17.10

RUN apt-get update -qq
RUN apt-get install -qq -y libtool intltool clang automake autoconf cmake
RUN apt-get install -qq -y libavformat-dev libavcodec-dev libavutil-dev libgtk-3-dev libsdl2-dev libxtst-dev libxml2-utils libxml2-dev libarchive-dev libcdio-dev

RUN mkdir -p /src/build/
WORKDIR /src/build/

COPY . /src/
RUN cmake .. -DCMAKE_C_COMPILER='clang' -DCMAKE_CXX_COMPILER='clang++' -DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX='/usr' -DCMAKE_INSTALL_LIBDIR='/usr/lib' -DSND_BACKEND='sdl' -DENABLE_CCDDA='ON' -DUSE_LIBARCHIVE='ON' -DUSE_LIBCDIO='ON'
RUN make
