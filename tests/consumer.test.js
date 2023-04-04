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

const Pulsar = require('../index.js');

(() => {
  describe('Consumer', () => {
    let client;

    beforeAll(() => {
      client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
    });

    afterAll(async () => {
      await client.close();
    });

    describe('Create', () => {
      test('No Topic', async () => {
        await expect(client.subscribe({
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Topic, topics or topicsPattern is required and must be specified as a string when creating consumer');
      });

      test('Not String Topic', async () => {
        await expect(client.subscribe({
          topic: 0,
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Topic, topics or topicsPattern is required and must be specified as a string when creating consumer');
      });

      test('Not String TopicsPattern', async () => {
        await expect(client.subscribe({
          topicsPattern: 0,
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Topic, topics or topicsPattern is required and must be specified as a string when creating consumer');
      });

      test('Not Array Topics', async () => {
        await expect(client.subscribe({
          topics: 0,
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Topic, topics or topicsPattern is required and must be specified as a string when creating consumer');
      });

      test('Not String in Array Topics', async () => {
        await expect(client.subscribe({
          topics: [0, true],
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Topic, topics or topicsPattern is required and must be specified as a string when creating consumer');
      });

      test('No Subscription', async () => {
        await expect(client.subscribe({
          topic: 'persistent://public/default/t1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Subscription is required and must be specified as a string when creating consumer');
      });

      test('Not String Subscription', async () => {
        await expect(client.subscribe({
          topic: 'persistent://public/default/t1',
          subscription: 0,
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Subscription is required and must be specified as a string when creating consumer');
      });

      test('Not Exist Tenant', async () => {
        await expect(client.subscribe({
          topic: 'persistent://no-tenant/namespace/topic',
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Failed to create consumer: BrokerMetadataError');
      });

      test('Not Exist Namespace', async () => {
        await expect(client.subscribe({
          topic: 'persistent://public/no-namespace/topic',
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Failed to create consumer: BrokerMetadataError');
      });

      test('Not Positive NAckRedeliverTimeout', async () => {
        await expect(client.subscribe({
          topic: 'persistent://public/default/t1',
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: -12,
        })).rejects.toThrow('NAck timeout should be greater than or equal to zero');
      });
    });

    describe('Close', () => {
      test('throws error on subsequent calls to close', async () => {
        const consumer = await client.subscribe({
          topic: 'persistent://public/default/my-topic',
          subscription: 'sub1',
          subscriptionType: 'Shared',
          // Test with listener since it changes the flow of close
          // and reproduces an issue
          listener() {},
        });

        await expect(consumer.close()).resolves.toEqual(null);

        await expect(consumer.close()).rejects.toThrow('Failed to close consumer: AlreadyClosed');
      });
    });

    describe('Listener', () => {
      test('share consumers with message listener', async () => {
        const topic = 'test-shared-consumer-listener-2';
        const producer = await client.createProducer({
          topic,
          batchingEnabled: false,
        });

        for (let i = 0; i < 100; i += 1) {
          await producer.send(i);
        }

        let consumer1Recv = 0;

        const consumer1 = await client.subscribe({
          topic,
          subscription: 'sub',
          subscriptionType: 'Shared',
          subscriptionInitialPosition: 'Earliest',
          receiverQueueSize: 10,
          listener: async (message, messageConsumer) => {
            await new Promise((resolve) => setTimeout(resolve, 10));
            consumer1Recv += 1;
            await consumer1.acknowledge(message);
          },
        });

        const consumer2 = await client.subscribe({
          topic,
          subscription: 'sub',
          subscriptionType: 'Shared',
          subscriptionInitialPosition: 'Earliest',
          receiverQueueSize: 10,
        });


        let consumer2Recv = 0;
        while (true) {
          await new Promise((resolve) => setTimeout(resolve, 10));
          try {
            const msg = await consumer2.receive(3000);
            consumer2Recv += 1;
            await consumer2.acknowledge(msg);
          } catch (err) {
            break;
          }
        }

        // Ensure that each consumer receives at least 1 times (greater than and not equal)
        // the receiver queue size messages.
        // This way any of the consumers will not immediately empty all messages of a topic.
        expect(consumer1Recv).toBeGreaterThan(10);
        expect(consumer1Recv).toBeGreaterThan(10);

        await consumer1.close();
        await consumer2.close();
        await producer.close();
      });
    });
  });
})();
