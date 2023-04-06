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

export PULSAR_STANDALONE_CONF=test-conf/standalone.conf
# There is an issue when starting the pulsar standalone with other metadata store: https://github.com/apache/pulsar/issues/17984
# We need to use Zookeeper here. Otherwise the `Message Chunking` test will not pass.
# We can remove this line after the pulsar release a new version contains this fix: https://github.com/apache/pulsar/pull/18126
export PULSAR_STANDALONE_USE_ZOOKEEPER=1
bin/pulsar-daemon start standalone \
        --no-functions-worker --no-stream-storage \
        --bookkeeper-dir data/bookkeeper

echo "-- Wait for Pulsar service to be ready"
for i in $(seq 30); do
    curl http://localhost:8080/metrics > /dev/null 2>&1 && break
    if [ $i -lt 30 ]; then
        sleep 1
    else
        echo '-- Pulsar standalone server startup timed out'
        exit 1
    fi
done

echo "-- Ready to start tests"
