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

const Pulsar = require('../index');
const { pulsar } = require('./protobuf_schema/generated/user_event_pb');
const userEventRootJson = require('./protobuf_schema/generated/user_event_root.json');

(() => {
  describe('ProtobufNativeSchema', () => {
    test('produce and consume protobuf native messages', async () => {
      const topic = `persistent://public/default/protobuf-native-schema-${Date.now()}`;
      const subscription = `protobuf-native-sub-${Date.now()}`;
      const userEventRoot = Pulsar.ProtobufNativeSchema.createRootFromJson(userEventRootJson);
      const schema = Pulsar.ProtobufNativeSchema.createSchemaInfoFromRoot({
        root: userEventRoot,
        rootMessageTypeName: 'pulsar.example.UserEvent',
        rootFileDescriptorName: 'user_event.proto',
      });

      expect(schema.schemaType).toBe('ProtobufNative');

      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      let producer;
      let consumer;

      try {
        consumer = await client.subscribe({
          topic,
          subscription,
          subscriptionType: 'Shared',
          ackTimeoutMs: 10000,
          schema,
        });

        producer = await client.createProducer({
          topic,
          sendTimeoutMs: 30000,
          batchingEnabled: true,
          schema,
        });

        const sent = [];
        for (let i = 0; i < 5; i += 1) {
          const userEvent = pulsar.example.UserEvent.create({
            id: `user-${i}`,
            name: `User ${i}`,
            age: 20 + i,
            tags: ['nodejs', 'protobuf'],
          });
          const data = Buffer.from(pulsar.example.UserEvent.encode(userEvent).finish());
          sent.push(pulsar.example.UserEvent.toObject(userEvent));
          await producer.send({ data });
        }
        await producer.flush();

        const received = [];
        for (let i = 0; i < sent.length; i += 1) {
          const msg = await consumer.receive();
          const userEvent = pulsar.example.UserEvent.decode(msg.getData());
          received.push(pulsar.example.UserEvent.toObject(userEvent));
          await consumer.acknowledge(msg);
        }

        expect(received).toEqual(sent);
      } finally {
        if (producer) {
          await producer.close();
        }
        if (consumer) {
          await consumer.close();
        }
        await client.close();
      }
    }, 30000);
  });
})();
