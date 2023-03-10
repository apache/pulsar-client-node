#!/bin/bash
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

# Test if the Pulsar.node built from ../pkg/linux/build-napi-inside-docker.sh
# can be loaded in other Linux distributions.

set -e

if [[ $# -lt 2 ]]; then
    echo "Usage $0 <node-image-name> <platform>

See https://hub.docker.com/_/node for valid images"
    exit 1
fi
IMAGE=$1
PLATFORM=$2

ROOT_DIR=${ROOT_DIR:-$(git rev-parse --show-toplevel)}
cd $ROOT_DIR

git archive -o pulsar-client-node.tar.gz HEAD

docker run --platform $PLATFORM -v $PWD:/pulsar-client-node $IMAGE \
    sh /pulsar-client-node/tests/docker-load-test.sh

rm pulsar-client-node.tar.gz
