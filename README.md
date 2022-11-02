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

This library works only in Node.js 10.x or later because it uses the
[node-addon-api](https://github.com/nodejs/node-addon-api) module to wrap the C++ library.


## Prebuilt binaries

The module uses [node-pre-gyp](https://github.com/mapbox/node-pre-gyp) to download the prebuilt binary for your platform, if it exists.
These binaries are hosted on ASF dist subversion. The following targets are currently provided:

Format: `napi-{platform}-{libc}-{arch}`
- napi-darwin-unknown-x64.tar.gz
- napi-linux-glibc-arm64.tar.gz
- napi-linux-glibc-x64.tar.gz
- napi-linux-musl-arm64.tar.gz
- napi-linux-musl-x64.tar.gz
- napi-win32-unknown-ia32.tar.gz
- napi-win32-unknown-x64.tar.gz

`darwin-arm64` systems are not currently supported, you can refer `How to build` to build from source.

## How to install

> **Note**
>
> These instructions are only available for versions after 1.8.0. For versions previous to 1.8.0, you need to install the C++ client first. Please switch to the corresponding version branch of this repo to read the specific instructions.

### Use `npm`

```shell
npm install pulsar-client
```

### Use `yarn`

```shell
yarn add pulsar-client
```

After install, you can run the [examples](https://github.com/apache/pulsar-client-node/tree/master/examples).

## How to build

### 1. Clone repository.
```shell
git clone https://github.com/apache/pulsar-client-node.git
cd pulsar-client-node
```

### 2. Install C++ client.

Select the appropriate installation method from below depending on your operating system:

Install C++ client on macOS:
```shell
pkg/mac/build-cpp-deps-lib.sh
pkg/mac/build-cpp-lib.sh
```

Install C++ client on Linux:
```shell
build-support/install-cpp-client.sh
```

Install C++ client on Windows (required preinstall `curl` and `7z`):
```shell
pkg\windows\download-cpp-client.bat
```

### 3. Build NAPI from source

```shell
npm install --build-from-source 
```


## Documentation
* Please see https://pulsar.apache.org/docs/client-libraries-node/ for more details about the Pulsar Node.js client.  
