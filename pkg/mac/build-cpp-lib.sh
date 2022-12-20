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

PULSAR_DIR=${MAC_BUILD_DIR}/build-pulsar
PULSAR_PREFIX=${PULSAR_DIR}/install
mkdir -p $PULSAR_PREFIX
cd $PULSAR_DIR

## Fetch from official release
curl -O -L "$CPP_CLIENT_BASE_URL"/apache-pulsar-client-cpp-${CPP_CLIENT_VERSION}.tar.gz
tar xfz apache-pulsar-client-cpp-${CPP_CLIENT_VERSION}.tar.gz
pushd apache-pulsar-client-cpp-${CPP_CLIENT_VERSION}
  rm -f CMakeCache.txt
  cmake . \
      -DCMAKE_OSX_ARCHITECTURES=${ARCH} \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=${MACOSX_DEPLOYMENT_TARGET} \
      -DCMAKE_INSTALL_PREFIX=$PULSAR_PREFIX \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=$PREFIX \
      -DCMAKE_INSTALL_LIBDIR=$PULSAR_PREFIX/lib \
      -DLINK_STATIC=ON \
      -DBUILD_TESTS=OFF \
      -DBUILD_PYTHON_WRAPPER=OFF \
      -DBUILD_DYNAMIC_LIB=OFF \
      -DPROTOC_PATH=$PREFIX/bin/protoc
  make -j16 install
  mkdir -p $ROOT_DIR/pkg/lib/
  cp -r lib/libpulsarwithdeps.a $ROOT_DIR/pkg/lib/
popd

rm -rf apache-pulsar-client-cpp-${CPP_CLIENT_VERSION}.tar.gz apache-pulsar-client-cpp-${CPP_CLIENT_VERSION}

