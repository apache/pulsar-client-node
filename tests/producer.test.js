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
const httpRequest = require('./http_utils');

const adminUrl = 'http://localhost:8080';

(() => {
  describe('Producer', () => {
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
        await expect(client.createProducer({
          sendTimeoutMs: 30000,
          batchingEnabled: true,
        })).rejects.toThrow('Topic is required and must be specified as a string when creating producer');
      });

      test('Not String Topic', async () => {
        await expect(client.createProducer({
          topic: 0,
          sendTimeoutMs: 30000,
          batchingEnabled: true,
        })).rejects.toThrow('Topic is required and must be specified as a string when creating producer');
      });

      test('Not Exist Tenant', async () => {
        await expect(client.createProducer({
          topic: 'persistent://no-tenant/namespace/topic',
          sendTimeoutMs: 30000,
          batchingEnabled: true,
        })).rejects.toThrow('Failed to create producer: TopicNotFound');
      });

      test('Not Exist Namespace', async () => {
        await expect(client.createProducer({
          topic: 'persistent://public/no-namespace/topic',
          sendTimeoutMs: 30000,
          batchingEnabled: true,
        })).rejects.toThrow('Failed to create producer: TopicNotFound');
      });

      test('Automatic Producer Name', async () => {
        const producer = await client.createProducer({
          topic: 'persistent://public/default/topic',
        });

        expect(typeof producer.getProducerName()).toBe('string');
        await producer.close();
      });

      test('Explicit Producer Name', async () => {
        const producer = await client.createProducer({
          topic: 'persistent://public/default/topic',
          producerName: 'test-producer',
        });

        expect(producer.getProducerName()).toBe('test-producer');
        await producer.close();
      });

      test('Topic Name', async () => {
        const producer = await client.createProducer({
          topic: 'persistent://public/default/topic',
        });

        expect(producer.getTopic()).toBe('persistent://public/default/topic');
        await producer.close();
      });
    });
    describe('Access Mode', () => {
      test('Exclusive', async () => {
        const topicName = 'test-access-mode-exclusive';
        const producer1 = await client.createProducer({
          topic: topicName,
          producerName: 'p-1',
          accessMode: 'Exclusive',
        });
        expect(producer1.getProducerName()).toBe('p-1');

        await expect(client.createProducer({
          topic: topicName,
          producerName: 'p-2',
          accessMode: 'Exclusive',
        })).rejects.toThrow('Failed to create producer: ResultProducerFenced');

        await producer1.close();
      });

      test('WaitForExclusive', async () => {
        const topicName = 'test-access-mode-wait-for-exclusive';
        const producer1 = await client.createProducer({
          topic: topicName,
          producerName: 'p-1',
          accessMode: 'Exclusive',
        });
        expect(producer1.getProducerName()).toBe('p-1');
        // async close producer1
        producer1.close();
        // when p1 close, p2 success created.
        const producer2 = await client.createProducer({
          topic: topicName,
          producerName: 'p-2',
          accessMode: 'WaitForExclusive',
        });
        expect(producer2.getProducerName()).toBe('p-2');
        await producer2.close();
      });

      test('ExclusiveWithFencing', async () => {
        const topicName = 'test-access-mode';
        const producer1 = await client.createProducer({
          topic: topicName,
          producerName: 'p-1',
          accessMode: 'Exclusive',
        });
        expect(producer1.getProducerName()).toBe('p-1');
        const producer2 = await client.createProducer({
          topic: topicName,
          producerName: 'p-2',
          accessMode: 'ExclusiveWithFencing',
        });
        expect(producer2.getProducerName()).toBe('p-2');
        // producer1 will be fenced.
        await expect(
          producer1.send({
            data: Buffer.from('test-msg'),
          }),
        ).rejects.toThrow('Failed to send message: ResultProducerFenced');
        await producer2.close();
      });
    });
    describe('Message Routing', () => {
      test('Custom Message Router', async () => {
        const topic = `test-custom-router-${Date.now()}`;
        const numPartitions = 3;

        // Create a partitioned topic via admin REST API
        const partitionedTopicAdminURL = `${adminUrl}/admin/v2/persistent/public/default/${topic}/partitions`;
        const response = await httpRequest(
          partitionedTopicAdminURL, {
            headers: {
              'Content-Type': 'application/json',
            },
            data: numPartitions,
            method: 'PUT',
          },
        );
        expect(response.statusCode).toBe(204);

        const producer = await client.createProducer({
          topic,
          batchingMaxMessages: 2,
          messageRouter: (message, topicMetadata) => {
            console.log(`key: ${message.getPartitionKey()}, partitions: ${topicMetadata.numPartitions}`);
            return parseInt(message.getPartitionKey(), 10) % topicMetadata.numPartitions;
          },
          messageRoutingMode: 'CustomPartition',
        });

        const promises = [];
        const numMessages = 5;
        for (let i = 0; i < numMessages; i += 1) {
          const sendPromise = producer.send({
            partitionKey: `${i}`,
            data: Buffer.from(`msg-${i}`),
          });
          await sendPromise;
          promises.push(sendPromise);
        }
        try {
          const allMsgIds = await Promise.all(promises);
          console.log(`All messages have been sent. IDs: ${allMsgIds.join(', ')}`);
          for (let i = 0; i < allMsgIds.length; i += 1) {
            // The message id string is in the format of "entryId,ledgerId,partition,batchIndex"
            const partition = Number(allMsgIds[i].toString().split(',')[2]);
            expect(i % numPartitions).toBe(partition);
          }
        } catch (error) {
          console.error('One or more messages failed to send:', error);
        }
      }, 30000);
    });
  });
})();
