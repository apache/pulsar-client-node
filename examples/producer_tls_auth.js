/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

const Pulsar = require('../');

(async () => {
  const auth = new Pulsar.AuthenticationTls({
    certificatePath: '/path/to/client.crt',
    privateKeyPath: '/path/to/client.key',
  });

  // Create a client
  const client = new Pulsar.Client({
    serviceUrl: 'pulsar+ssl://localhost:6651',
    authentication: auth,
    tlsTrustCertsFilePath: '/path/to/server.crt',
  });

  // Create a producer
  const producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
  });

  // Send messages
  for (let i = 0; i < 10; i += 1) {
    const msg = `my-message-${i}`;
    producer.send({
      data: Buffer.from(msg),
    });
    console.log(`Sent message: ${msg}`);
  }
  await producer.flush();

  await producer.close();
  await client.close();
})();
