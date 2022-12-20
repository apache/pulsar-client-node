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

ROOT_DIR=`cd $(dirname $0) && cd .. && pwd`
source $ROOT_DIR/pulsar-client-cpp.txt

if [ $USER != "root" ]; then
  SUDO="sudo"
fi

# Get the flavor of Linux
export $(cat /etc/*-release | grep "^ID=")

cd /tmp

UNAME_ARCH=$(uname -m)
if [ $UNAME_ARCH == 'aarch64' ]; then
  PLATFORM=arm64
else
  PLATFORM=x86_64
fi

if [ $ID == 'ubuntu' -o $ID == 'debian' ]; then
  curl -L -O ${CPP_CLIENT_BASE_URL}/deb-${PLATFORM}/apache-pulsar-client.deb
  curl -L -O ${CPP_CLIENT_BASE_URL}/deb-${PLATFORM}/apache-pulsar-client-dev.deb
  $SUDO apt install -y /tmp/*.deb

elif [ $ID == 'alpine' ]; then
  curl -L -O ${CPP_CLIENT_BASE_URL}/apk-${PLATFORM}/${UNAME_ARCH}/apache-pulsar-client-${CPP_CLIENT_VERSION}-r0.apk
  curl -L -O ${CPP_CLIENT_BASE_URL}/apk-${PLATFORM}/${UNAME_ARCH}/apache-pulsar-client-dev-${CPP_CLIENT_VERSION}-r0.apk
  $SUDO apk add --allow-untrusted /tmp/*.apk

elif [ $ID == '"centos"' ]; then
  curl -L -O ${CPP_CLIENT_BASE_URL}/rpm-${PLATFORM}/${UNAME_ARCH}/apache-pulsar-client-${CPP_CLIENT_VERSION}-1.${UNAME_ARCH}.rpm
  curl -L -O ${CPP_CLIENT_BASE_URL}/rpm-${PLATFORM}/${UNAME_ARCH}/apache-pulsar-client-devel-${CPP_CLIENT_VERSION}-1.${UNAME_ARCH}.rpm
  $SUDO rpm -i /tmp/*.rpm

else
  echo "Unknown Linux distribution: '$ID'"
  exit 1
fi

mkdir -p $ROOT_DIR/pkg/lib/
cp /usr/lib/libpulsarwithdeps.a $ROOT_DIR/pkg/lib/



