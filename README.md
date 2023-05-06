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

The Pulsar Node.js client can be used to create Pulsar producers and consumers in Node.js. For the supported Pulsar features, see [Client Feature Matrix](https://pulsar.apache.org/client-feature-matrix/).

This library works only in Node.js 10.x or later because it uses the
[node-addon-api](https://github.com/nodejs/node-addon-api) module to wrap the C++ library.

## Getting Started

> **Note**
>
> These instructions are only available for versions after 1.8.0. For versions previous to 1.8.0, you need to install the C++ client first. Please switch to the corresponding version branch of this repo to read the specific instructions.
>
> To run the examples, skip this section.

To use the Pulsar Node.js client in your project, run:

```shell
npm install pulsar-client
```

or

```shell
yarn add pulsar-client
```

Then you can run the following simple end-to-end example:

```javascript
const Pulsar = require('pulsar-client');

(async () => {
  // Create a client
  const client = new Pulsar.Client({
    serviceUrl: 'pulsar://localhost:6650'
  });

  // Create a producer
  const producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
  });

  // Create a consumer
  const consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'sub1'
  });

  // Send a message
  producer.send({
    data: Buffer.from("hello")
  });

  // Receive the message
  const msg = await consumer.receive();
  console.log(msg.getData().toString());
  consumer.acknowledge(msg);

  await producer.close();
  await consumer.close();
  await client.close();
})();
```

You should find the output as:

```
hello
```

You can see more examples in the [examples](./examples) directory. However, since these examples might use an API that was not released yet, you need to build this module. See the next section.

## How to build

> **Note**
>
> Building from source code requires a Node.js version greater than 16.18.

First, clone the repository.

```shell
git clone https://github.com/apache/pulsar-client-node.git
cd pulsar-client-node
```

Since this client is a [C++ addon](https://nodejs.org/api/addons.html#c-addons) that depends on the [Pulsar C++ client](https://github.com/apache/pulsar-client-cpp), you need to install the C++ client first. You need to ensure there is a C++ compiler that supports C++11 installed in your system.

- Install C++ client on Linux:

```shell
pkg/linux/download-cpp-client.sh
```

- Install C++ client on Windows:

```shell
pkg\windows\download-cpp-client.bat
```

- Install C++ client on macOS:

```shell
pkg/mac/build-cpp-deps-lib.sh
pkg/mac/build-cpp-lib.sh
```

After the C++ client is installed, run the following command to build this C++ addon.

```shell
npm install
```

To verify it has been installed successfully, you can run an example like:

> **Note**
>
> A running Pulsar server is required. The example uses `pulsar://localhost:6650` to connect to the server.

```shell
node examples/producer
```

You should find the output as:

```
Sent message: my-message-0
Sent message: my-message-1
Sent message: my-message-2
Sent message: my-message-3
Sent message: my-message-4
Sent message: my-message-5
Sent message: my-message-6
Sent message: my-message-7
Sent message: my-message-8
Sent message: my-message-9
```

## Documentation

For more details about Pulsar Node.js clients, see [Pulsar docs](https://pulsar.apache.org/docs/client-libraries-node/).

### Contribute

Contributions are welcomed and greatly appreciated.

If your contribution adds Pulsar features for Node.js clients, you need to update both the [Pulsar docs](https://pulsar.apache.org/docs/client-libraries/) and the [Client Feature Matrix](https://pulsar.apache.org/client-feature-matrix/). See [Contribution Guide](https://pulsar.apache.org/contribute/site-intro/#pages) for more details.

### Generate API docs

```shell
npm install
npx typedoc
# Documentation generated at ./apidocs
```
