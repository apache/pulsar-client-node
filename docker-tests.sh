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

ROOT_DIR=${ROOT_DIR:-$(git rev-parse --show-toplevel)}
cd $ROOT_DIR

BUILD_IMAGE_NAME="${BUILD_IMAGE_NAME:-apachepulsar/pulsar-build}"
BUILD_IMAGE_VERSION="${BUILD_IMAGE_VERSION:-ubuntu-20.04}"

IMAGE="$BUILD_IMAGE_NAME:$BUILD_IMAGE_VERSION"

echo "---- Testing Pulsar node client using image $IMAGE"

docker pull $IMAGE

TARGET_DIR=/pulsar-client-node
DOCKER_CMD="docker run -i -e ROOT_DIR=$TARGET_DIR -v $ROOT_DIR:$TARGET_DIR $IMAGE"

# Start Pulsar standalone instance
# and execute the tests
$DOCKER_CMD bash -c "cd /pulsar-client-node && ./run-unit-tests.sh"
