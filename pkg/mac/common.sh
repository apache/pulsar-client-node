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

set -e -x

if [ -z "$ARCH" ]; then
   export ARCH=$(uname -m)
fi

export MACOSX_DEPLOYMENT_TARGET=11.0

MAC_BUILD_DIR=`cd $(dirname $0); pwd`
ROOT_DIR=$(git rev-parse --show-toplevel)
source $ROOT_DIR/pulsar-client-cpp.txt

cd $MAC_BUILD_DIR
mkdir -p build
cd build
mkdir -p install
export PREFIX=`pwd`/install

