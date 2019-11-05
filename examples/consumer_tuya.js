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

//const Pulsar = require('pulsar-client');
const Pulsar = require('../index.js');

(async () => {
  // Create a client
      
  const auth = new Pulsar.AuthenticationTuya({
    accessId: '84dj9ppwvvdyrf4e4sxq',
    accessKey: 'yfcmuthtghmvhscj8redgp9r3wwsnr5w',
  });
  const client = new Pulsar.Client({
    serviceUrl: "pulsar+ssl://mqe.tuyaus.com:7285/",
    authentication: auth,
    tlsAllowInsecureConnection: true,
  });
  // Create a consumer
   const consumer = await client.subscribe({
    topic: '84dj9ppwvvdyrf4e4sxq/out/event',
    subscription: '84dj9ppwvvdyrf4e4sxq-sub',
    ackTimeoutMs: 10000,
   });
  // Receive messages
     
  for (let i = 0; i < 10000; i += 1) {
    const msg = await consumer.receive();
    console.log(msg.getData().toString());
    consumer.acknowledge(msg);
  }
  await consumer.close();
  await client.close();
 
})();
