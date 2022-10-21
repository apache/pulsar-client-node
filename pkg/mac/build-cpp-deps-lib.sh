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

ZLIB_VERSION=1.2.13
OPENSSL_VERSION=1_1_1q
BOOST_VERSION=1.79.0
PROTOBUF_VERSION=3.20.0
ZSTD_VERSION=1.5.2
SNAPPY_VERSION=1.1.3
CURL_VERSION=7.61.0

###############################################################################
if [ ! -f zlib-${ZLIB_VERSION}/.done ]; then
    echo "Building ZLib"
    curl -O -L https://zlib.net/fossils/zlib-${ZLIB_VERSION}.tar.gz
    tar xfz zlib-$ZLIB_VERSION.tar.gz
    pushd zlib-$ZLIB_VERSION
      CFLAGS="-fPIC -O3 -arch ${ARCH} -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}" ./configure --prefix=$PREFIX
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
    curl -O -L https://github.com/openssl/openssl/archive/refs/heads/OpenSSL_1_1_1-stable.zip
    unzip OpenSSL_1_1_1-stable.zip
    pushd openssl-OpenSSL_1_1_1-stable
        if [ $ARCH = 'arm64' ]; then
          PLATFORM=darwin64-arm64-cc
        else
          PLATFORM=darwin64-x86_64-cc
        fi
        ./Configure --prefix=$PREFIX no-shared no-unit-test $PLATFORM
        make -j8
        make install_sw
    popd

    rm -rf OpenSSL_${OPENSSL_VERSION}.tar.gz openssl-OpenSSL_${OPENSSL_VERSION}
    touch openssl-OpenSSL_${OPENSSL_VERSION}.done
else
    echo "Using cached OpenSSL"
fi

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
      ./b2 address-model=64 cxxflags="-fPIC -arch ${ARCH} -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}" \
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
      CXXFLAGS="-fPIC -arch arm64 -arch x86_64 -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}" \
            ./configure --prefix=$PREFIX
      make -j16 V=1
      make install
    popd

    pushd install/lib
      echo "Propose target arch static lib" ${ARCH}
      mv libprotobuf.a libprotobuf_universal.a
      lipo libprotobuf_universal.a -thin ${ARCH} -output libprotobuf.a
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
      CFLAGS="-fPIC -O3 -arch ${ARCH} -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}" \
          PREFIX=$PREFIX  \
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
      CXXFLAGS="-fPIC -O3 -arch ${ARCH} -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}" \
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
      if [ $ARCH = 'arm64' ]; then
        HOST=aarch64
      else
        HOST=x86_64
      fi
      CFLAGS="-fPIC -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}" \
        ./configure --with-ssl=$PREFIX \
                --without-nghttp2 \
                --without-libidn2 \
                --disable-ldap \
                --without-librtmp \
                --without-brotli \
                --prefix=$PREFIX \
                --host=$HOST
      make -j16 install
    popd

    rm -rf curl-${CURL_VERSION} curl-${CURL_VERSION}.tar.gz
    touch curl-${CURL_VERSION}.done
else
    echo "Using cached LibCurl"
fi
