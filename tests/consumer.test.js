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
        })).rejects.toThrow('Failed to create consumer: TopicNotFound');
      });

      test('Not Exist Namespace', async () => {
        await expect(client.subscribe({
          topic: 'persistent://public/no-namespace/topic',
          subscription: 'sub1',
          ackTimeoutMs: 10000,
          nAckRedeliverTimeoutMs: 60000,
        })).rejects.toThrow('Failed to create consumer: TopicNotFound');
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

    describe('Features', () => {
      test('Batch index ack', async () => {
        const topicName = 'test-batch-index-ack';
        const producer = await client.createProducer({
          topic: topicName,
          batchingEnabled: true,
          batchingMaxMessages: 100,
          batchingMaxPublishDelayMs: 10000,
        });

        let consumer = await client.subscribe({
          topic: topicName,
          batchIndexAckEnabled: true,
          subscription: 'test-batch-index-ack',
        });

        // Make sure send 0~5 is a batch msg.
        for (let i = 0; i < 5; i += 1) {
          const msg = `my-message-${i}`;
          console.log(msg);
          producer.send({
            data: Buffer.from(msg),
          });
        }
        await producer.flush();

        // Receive msgs and just ack 0, 1 msgs
        const results = [];
        for (let i = 0; i < 5; i += 1) {
          const msg = await consumer.receive();
          results.push(msg);
        }
        expect(results.length).toEqual(5);
        for (let i = 0; i < 2; i += 1) {
          await consumer.acknowledge(results[i]);
          await new Promise((resolve) => setTimeout(resolve, 200));
        }

        // Restart consumer after, just receive 2~5 msg.
        await consumer.close();
        consumer = await client.subscribe({
          topic: topicName,
          batchIndexAckEnabled: true,
          subscription: 'test-batch-index-ack',
        });
        const results2 = [];
        for (let i = 2; i < 5; i += 1) {
          const msg = await consumer.receive();
          results2.push(msg);
        }
        expect(results2.length).toEqual(3);
        // assert no more msgs.
        await expect(consumer.receive(1000)).rejects.toThrow(
          'Failed to receive message: TimeOut',
        );
      });
    });
  });
})();
