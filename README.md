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
| 1.0.0          | 2.3.0 or later |
| 1.1.0          | 2.4.0 or later |
| 1.2.0          | 2.5.0 or later |

If an incompatible version of the C++ client is installed, you may fail to build or run this library.

## How to install

### Please install pulsar-client in your project:

```shell
$ npm install pulsar-client
```

## Typescript Definitions

```shell
$ npm install @types/pulsar-client --save-dev
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

### Rebuild Pulsar client library:

```shell
$ npm run build
```
