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

## How to install

> **Note**
>
> Only available for versions after 1.8.0. For versions before 1.8.0, you need to install the C++ client first, and switch to the corresponding version branch to view the specific steps.

1. You can use `npm` or `yarn` to install `pulsar-client-node`

```shell
npm install pulsar-client
#or
yarn add pulsar-client
```

2. Run Sample code

Please refer to [examples](https://github.com/apache/pulsar-client-node/tree/master/examples).


### Prebuilt binaries

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

## How to build

To build from source, you need to install the CPP client first.

1. Clone repository.
```shell
$ git clone https://github.com/apache/pulsar-client-node.git
$ cd pulsar-client-node
```

2. Install CPP client.

Select the appropriate installation method from below depending on your operating system.

Install c++ client on mac
```shell
$ pkg/mac/build-cpp-deps-lib.sh
$ pkg/mac/build-cpp-lib.sh
```

Install c++ client on Linux
```shell
$ build-support/install-cpp-client.sh
```

Install c++ client on Windows(Need install curl and 7z first)
```shell
$ pkg\windows\download-cpp-client.bat
```

3. Build NAPI from source.

```shell
npm install --build-from-source 
```


## Documentation
* Please see https://pulsar.apache.org/docs/en/client-libraries-node for more details about the Pulsar Node.js client.  
