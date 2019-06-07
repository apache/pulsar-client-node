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

set -e

ROOT_DIR=$(git rev-parse --show-toplevel)
cd $ROOT_DIR

VERSION="${VERSION:-`cat ./pulsar-version.txt`}"
PULSAR_DIR="${PULSAR_DIR:-/tmp/pulsar-test-dist}"
PKG=apache-pulsar-${VERSION}-bin.tar.gz

rm -rf $PULSAR_DIR
curl -L --create-dir "https://archive.apache.org/dist/pulsar/pulsar-${VERSION}/${PKG}" -o $PULSAR_DIR/$PKG
tar xfz $PULSAR_DIR/$PKG -C $PULSAR_DIR --strip-components 1

DATA_DIR=/tmp/pulsar-test-data
rm -rf $DATA_DIR
mkdir -p $DATA_DIR

export PULSAR_STANDALONE_CONF=$ROOT_DIR/tests/conf/standalone.conf
$PULSAR_DIR/bin/pulsar-daemon start standalone \
        --no-functions-worker --no-stream-storage \
        --zookeeper-dir $DATA_DIR/zookeeper \
        --bookkeeper-dir $DATA_DIR/bookkeeper

echo "-- Wait for Pulsar service to be ready"
until curl http://localhost:8080/metrics > /dev/null 2>&1 ; do sleep 1; done
