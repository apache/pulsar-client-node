<!--

    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.

-->

# Pulsar Node.js client library

The Pulsar Node.js client can be used to create Pulsar producers and consumers in Node.js.

## Requirements

Pulsar Node.js client library is based on the C++ client library. Follow the instructions for
[C++ library](https://pulsar.apache.org/docs/en/client-libraries-cpp/) for installing the binaries through
[RPM](https://pulsar.apache.org/docs/en/client-libraries-cpp/#rpm),
[Deb](https://pulsar.apache.org/docs/en/client-libraries-cpp/#deb) or
[Homebrew packages](https://pulsar.apache.org/docs/en/client-libraries-cpp/#macos).

(Note: you will need to install not only the pulsar-client library but also the pulsar-client-dev library)

Also, this library works only in Node.js 10.x or later because it uses the
[node-addon-api](https://github.com/nodejs/node-addon-api) module to wrap the C++ library.

## Compatibility

Compatibility between each version of the Node.js client and the C++ client is as follows:

| Node.js client | C++ client     |
|----------------|----------------|
| 1.0.x          | 2.3.0 or later |
| 1.1.x          | 2.4.0 or later |
| 1.2.x          | 2.5.0 or later |
| 1.3.x          | 2.7.0 or later |
| 1.4.x - 1.6.x  | 2.8.0 or later |

If an incompatible version of the C++ client is installed, you may fail to build or run this library.

## How to install

### Install on windows

1. Build the Pulsar C++ client on windows.

```shell
cmake \
 -A x64 \
 -DBUILD_PYTHON_WRAPPER=OFF -DBUILD_TESTS=OFF \
 -DVCPKG_TRIPLET=x64-windows \
 -DCMAKE_BUILD_TYPE=Release \
 -S .
cmake --config Release
```


2. Set the variable `PULSAR_CPP_DIR` with the `pulsar-client-cpp` path in a Windows command tool.

```shell
# for example
set PULSAR_CPP_DIR=C:\pulsar\pulsar-client-cpp
```

3. Set the variable `OS_ARCH` in a Windows command tool, `OS_ARCH` is related to the configuration of VCPKG_TRIPLET on the command line above.(Optional)

```shell
set OS_ARCH=x64-windows
```

### Install on mac

1. Install the Pulsar C++ client on mac.

```shell
brew install libpulsar
```

2. Get the installation path of libpulsar

```shell
brew info libpulsar
```


2. Set the variable `PULSAR_CPP_DIR` with the `pulsar-client-cpp` path in a mac command tool.

```shell
# for example
export PULSAR_CPP_DIR=/usr/local/Cellar/libpulsar/2.9.1_1
```


### Install pulsar-client to your project

```shell
$ npm install pulsar-client
```

## Sample code

Please refer to [examples](https://github.com/apache/pulsar-client-node/tree/master/examples).

## How to build

### Install dependent npm modules and build Pulsar client library:

```shell
$ git clone https://github.com/apache/pulsar-client-node.git
$ cd pulsar-client-node
$ npm install
```

> **Note**
>
> If you build `pulsar-client-node on` windows, you need to set the variable `PULSAR_CPP_DIR` first, then install npm (run the command `npm install`) in a Windows command-line tool.

### Rebuild Pulsar client library:

```shell
$ npm run build
```
