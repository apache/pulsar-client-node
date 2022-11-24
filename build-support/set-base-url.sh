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

pushd "$( dirname -- "${BASH_SOURCE[0]}" )"
ROOT_DIR=$(git rev-parse --show-toplevel)
CPP_CLIENT_VERSION=$(< "$ROOT_DIR"/pulsar-client-cpp-version.txt xargs)

export BASE_URL=https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-cpp/pulsar-client-cpp-${CPP_CLIENT_VERSION}-candidate-1
popd
