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

if [ $# -neq 2 ]; then
    echo "Usage: $0 \$DEST_PATH \$WORKFLOW_ID"
    exit 1
fi

DEST_PATH=$(readlink -f $1)
WORKFLOW_ID=$2

pushd $(dirname "$0")
PULSAR_NODE_PATH=$(git rev-parse --show-toplevel)
popd

mkdir -p $DEST_PATH

cd $PULSAR_NODE_PATH

build-support/download-release-artifacts.py $WORKFLOW_ID $DEST_PATH
build-support/generate-source-archive.sh $DEST_PATH

# Sign all files
cd $DEST_PATH
find . -type f | xargs $PULSAR_NODE_PATH/build-support/sign-files.sh
