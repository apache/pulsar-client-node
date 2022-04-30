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

PULSAR_DIR=${DEPS_DIR}/build-pulsar
PULSAR_PREFIX=${PULSAR_DIR}/install
mkdir -p $PULSAR_PREFIX
cd $PULSAR_DIR

# Pulsar
curl -O -L "https://www.apache.org/dyn/mirrors/mirrors.cgi?action=download&filename=pulsar/pulsar-$PULSAR_VERSION/apache-pulsar-$PULSAR_VERSION-src.tar.gz"
tar xfz apache-pulsar-$PULSAR_VERSION-src.tar.gz
pushd apache-pulsar-$PULSAR_VERSION-src/pulsar-client-cpp
  chmod +x ./build-support/merge_archives.sh
  rm -f CMakeCache.txt
  cmake . \
      -DBUILD_PYTHON_WRAPPER=OFF \
      -DBUILD_DYNAMIC_LIB=ON \
      -DLINK_STATIC=ON \
      -DBUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$PULSAR_PREFIX \
      -DCMAKE_INSTALL_LIBDIR=$PULSAR_PREFIX/lib \
      -DCMAKE_PREFIX_PATH=$PREFIX \
      -DPROTOC_PATH=$PREFIX/bin/protoc
  make -j8 VERBOSE=1
  make install
  cp lib/libpulsarwithdeps.a $PULSAR_PREFIX/lib
popd

rm -rf apache-pulsar-$PULSAR_VERSION-src.tar.gz apache-pulsar-$PULSAR_VERSION-src

