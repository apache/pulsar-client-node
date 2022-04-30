#!/usr/bin/env bash
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

source $(dirname $0)/common.sh

ZLIB_VERSION=1.2.12
OPENSSL_VERSION=1_1_1q
BOOST_VERSION=1.79.0
PROTOBUF_VERSION=3.20.0
ZSTD_VERSION=1.5.2
SNAPPY_VERSION=1.1.3
CURL_VERSION=7.61.0

###############################################################################
if [ ! -f zlib-${ZLIB_VERSION}/.done ]; then
    echo "Building ZLib"
    curl -O -L https://zlib.net/zlib-${ZLIB_VERSION}.tar.gz
    tar xfz zlib-$ZLIB_VERSION.tar.gz
    pushd zlib-$ZLIB_VERSION
      CFLAGS="$CFLAGS $ARCH_FLAGS" ./configure --prefix=$PREFIX
      make -j16
      make install
      touch .done
    popd
else
    echo "Using cached ZLib"
fi

###############################################################################
if [ ! -f openssl-OpenSSL_${OPENSSL_VERSION}.done ]; then
    echo "Building OpenSSL"
    curl -O -L https://github.com/openssl/openssl/archive/OpenSSL_${OPENSSL_VERSION}.tar.gz
    tar xfz OpenSSL_${OPENSSL_VERSION}.tar.gz
    pushd openssl-OpenSSL_${OPENSSL_VERSION}
      if [ $IS_MACOS = '1' ]; then
        ./Configure --prefix=$PREFIX no-shared darwin64-arm64-cc
        make -j8

        cp libssl.a libssl-arm64.a
        cp libcrypto.a libcrypto-arm64.a

        make clean
        ./Configure --prefix=$PREFIX no-shared darwin64-x86_64-cc
        make -j8 && make install_sw

        # Create universal binaries
        lipo -create libssl-arm64.a libssl.a -output $PREFIX/lib/libssl.a
        lipo -create libcrypto-arm64.a libcrypto.a -output $PREFIX/lib/libcrypto.a

      else
        ## Linux
        if [ $IS_ARM = '1' ]; then
          PLATFORM=linux-aarch64
        else
          PLATFORM=linux-x86_64
        fi

        ./Configure --prefix=$PREFIX no-shared $PLATFORM
        make -j8
        make install_sw
      fi
    popd

    rm -rf OpenSSL_${OPENSSL_VERSION}.tar.gz openssl-OpenSSL_${OPENSSL_VERSION}
    touch openssl-OpenSSL_${OPENSSL_VERSION}.done
else
    echo "Using cached OpenSSL"
fi

export CFLAGS="$CFLAGS $ARCH_FLAGS"
export CXXFLAGS=$CFLAGS

###############################################################################
BOOST_VERSION_=${BOOST_VERSION//./_}
DIR=boost-src-${BOOST_VERSION}
if [ ! -f $DIR.done ]; then
    echo "Building Boost"
    curl -O -L https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION_}.tar.gz
    tar xfz boost_${BOOST_VERSION_}.tar.gz
    rm -rf $DIR
    mv boost_${BOOST_VERSION_} $DIR

    pushd $DIR
      ./bootstrap.sh --prefix=$PREFIX --with-libraries=system
      ./b2 address-model=64 cxxflags="$CXXFLAGS" \
                link=static threading=multi \
                variant=release \
                install

    popd
    
    touch $DIR.done

    rm -rf $DIR boost_${BOOST_VERSION_}.tar.gz
else
    echo "Using cached Boost"
fi

###############################################################################
if [ ! -f protobuf-${PROTOBUF_VERSION}.done ]; then
    echo "Building Protobuf"
    curl -O -L  https://github.com/google/protobuf/releases/download/v${PROTOBUF_VERSION}/protobuf-cpp-${PROTOBUF_VERSION}.tar.gz
    tar xfz protobuf-cpp-${PROTOBUF_VERSION}.tar.gz
    pushd protobuf-${PROTOBUF_VERSION}
      ./configure --prefix=$PREFIX $CONFIGURE_ARGS
      make -j16 V=1
      make install
    popd

    rm -rf protobuf-${PROTOBUF_VERSION} protobuf-cpp-${PROTOBUF_VERSION}.tar.gz
    touch protobuf-${PROTOBUF_VERSION}.done
else
    echo "Using cached Protobuf"
fi

###############################################################################
if [ ! -f zstd-${ZSTD_VERSION}.done ]; then
    echo "Building ZStd"
    curl -O -L https://github.com/facebook/zstd/releases/download/v${ZSTD_VERSION}/zstd-${ZSTD_VERSION}.tar.gz
    tar xfz zstd-${ZSTD_VERSION}.tar.gz
    pushd zstd-${ZSTD_VERSION}
      PREFIX=$PREFIX \
            make -j16 -C lib install
    popd
    
    touch zstd-${ZSTD_VERSION}.done
    rm -rf zstd-${ZSTD_VERSION} zstd-${ZSTD_VERSION}.tar.gz
else
    echo "Using cached ZStd"
fi

###############################################################################
if [ ! -f snappy-${SNAPPY_VERSION}.done ]; then
    echo "Building Snappy"
    curl -O -L https://github.com/google/snappy/releases/download/${SNAPPY_VERSION}/snappy-${SNAPPY_VERSION}.tar.gz
    tar xfz snappy-${SNAPPY_VERSION}.tar.gz
    pushd snappy-${SNAPPY_VERSION}
      ./configure --prefix=$PREFIX
      make -j16
      make install
    popd

    rm -rf snappy-${SNAPPY_VERSION} snappy-${SNAPPY_VERSION}.tar.gz
    touch snappy-${SNAPPY_VERSION}.done
else
    echo "Using cached Snappy"
fi

###############################################################################
if [ ! -f curl-${CURL_VERSION}.done ]; then
    echo "Building LibCurl"
    CURL_VERSION_=${CURL_VERSION//./_}
    curl -O -L  https://github.com/curl/curl/releases/download/curl-${CURL_VERSION_}/curl-${CURL_VERSION}.tar.gz
    tar xfz curl-${CURL_VERSION}.tar.gz
    pushd curl-${CURL_VERSION}
      ./configure --with-ssl=$PREFIX \
              --without-nghttp2 --without-libidn2 --disable-ldap \
              --without-librtmp --without-brotli \
              --prefix=$PREFIX
      make -j16 install
    popd

    rm -rf curl-${CURL_VERSION} curl-${CURL_VERSION}.tar.gz
    touch curl-${CURL_VERSION}.done
else
    echo "Using cached LibCurl"
fi
