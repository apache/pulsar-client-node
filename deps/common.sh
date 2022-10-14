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

export MACOSX_DEPLOYMENT_TARGET=11.0
MACOSX_DEPLOYMENT_TARGET_MAJOR=${MACOSX_DEPLOYMENT_TARGET%%.*}

set -e -x

DEPS_DIR=`cd $(dirname $0); pwd`
TOP_DIR=$DEPS_DIR/..
PULSAR_CPP_VERSION=`cat $TOP_DIR/pulsar-client-cpp-version.txt`

cd $DEPS_DIR


echo "system versin+++++++:"
uname -m

mkdir -p build
cd build

mkdir -p install
PREFIX=`pwd`/install

export CFLAGS="-fPIC -O3"

if [ $(uname) = "Darwin" ]; then
  sw_vers
  IS_MACOS=1
  export CFLAGS="$CFLAGS -mmacosx-version-min=${MACOSX_DEPLOYMENT_TARGET}"
else
  IS_MACOS=0
fi

if [ "$ARCH" = "arm64" ] || [ $(uname -p) = "arm" ] || [ $(uname -p) = "aarch64" ]; then
  IS_ARM=1
  export ARCH_FLAGS="-arch arm64"
  export CONFIGURE_ARGS="--host=arm64"
  export CONFIGURE_ARGS2="--host=aarch64"
else
  IS_ARM=0
fi

export CXXFLAGS="$CFLAGS"
