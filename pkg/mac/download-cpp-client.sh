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

set -e -x

ROOT_DIR=`cd $(dirname $0) && cd ../../ && pwd`
source $ROOT_DIR/pulsar-client-cpp.txt

if [ -z "$ARCH" ]; then
   export ARCH=$(uname -m)
fi

rm -rf $ROOT_DIR/pkg/mac/build-pulsar
mkdir -p $ROOT_DIR/pkg/mac/build-pulsar/install
cd $ROOT_DIR/pkg/mac
curl -L -O ${CPP_CLIENT_BASE_URL}/macos-${ARCH}.zip
unzip -d $ROOT_DIR/pkg/mac/build-pulsar/install macos-${ARCH}.zip
rm -rf macos-${ARCH}.zip

