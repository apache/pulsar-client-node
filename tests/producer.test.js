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
        // 1. Define a partitioned topic and a custom router
        const partitionedTopicName = `test-custom-router-${Date.now()}`;
        const partitionedTopic = `persistent://public/default/${partitionedTopicName}`;
        const numPartitions = 10;

        // Use admin client to create a partitioned topic. This is more robust.
        // Assuming 'adminUrl' and 'httpRequest' are available from your test setup.
        const partitionedTopicAdminURL = `${adminUrl}/admin/v2/persistent/public/default/${partitionedTopicName}/partitions`;
        const createPartitionedTopicRes = await httpRequest(
          partitionedTopicAdminURL, {
            headers: {
              'Content-Type': 'application/json', // Use application/json for REST API
            },
            data: numPartitions,
            method: 'PUT',
          },
        );
        // 204 No Content is success for PUT create
        expect(createPartitionedTopicRes.statusCode).toBe(204);

        const routingKey = 'user-id-12345';
        const simpleHash = (str) => {
          let hash = 0;
          /* eslint-disable no-bitwise */
          for (let i = 0; i < str.length; i += 1) {
            const char = str.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash &= hash;
          }
          /* eslint-disable no-bitwise */
          return Math.abs(hash);
        };
        const expectedPartition = simpleHash(routingKey) % numPartitions;
        console.log(`Routing key '${routingKey}' will be sent to partition: ${expectedPartition}`);

        // 2. Create a producer with the custom message router
        const producer = await client.createProducer({
          topic: partitionedTopic,
          messageRouter: (message, topicMetadata) => {
            // Get the routingKey from the message properties
            const key = message.properties.routingKey;
            if (key) {
              // Use the metadata to get the number of partitions
              const numPartitionsAvailable = topicMetadata.numPartitions;
              // Calculate the target partition
              return simpleHash(key) % numPartitionsAvailable;
            }
            // Fallback to a default partition if no key is provided
            return 0;
          },
          messageRoutingMode: 'CustomPartition',
        });

        // 3. Create a single consumer for the entire partitioned topic
        const consumer = await client.subscribe({
          topic: partitionedTopic,
          subscription: 'test-sub',
          subscriptionInitialPosition: 'Earliest',
        });

        // 4. Send 1000 messages in parallel for efficiency
        console.log(`Sending messages to partitioned topic ${partitionedTopic}...`);
        const numMessages = 1000;
        for (let i = 0; i < numMessages; i += 1) {
          const msg = `message-${i}`;
          producer.send({
            data: Buffer.from(msg),
            properties: {
              routingKey,
            },
          });
        }
        await producer.flush();
        console.log(`Sent ${numMessages} messages.`);

        // 5. Receive messages and assert they all come from the target partition
        const receivedMessages = new Set();
        const expectedPartitionName = `${partitionedTopic}-partition-${expectedPartition}`;

        for (let i = 0; i < numMessages; i += 1) {
          const msg = await consumer.receive(10000);
          // eslint-disable-next-line no-underscore-dangle
          expect(msg.getProperties().__partition__).toBe(String(expectedPartition));
          expect(msg.getTopicName()).toBe(expectedPartitionName);
          receivedMessages.add(msg.getData().toString());
          await consumer.acknowledge(msg);
        }
        // Final assertion to ensure all unique messages were received
        expect(receivedMessages.size).toBe(numMessages);
        console.log(`Successfully received and verified ${receivedMessages.size} messages from ${expectedPartitionName}.`);
        await producer.close();
        await consumer.close();
      }, 30000);

      test('Custom Message Router Exception', async () => {
        // 1. Define a partitioned topic and a custom router
        const partitionedTopicName = `test-custom-router-failed-${Date.now()}`;
        const partitionedTopic = `persistent://public/default/${partitionedTopicName}`;

        // 2. Create a producer with the custom message router
        const producer = await client.createProducer({
          topic: partitionedTopic, // Note: For producer, use the base topic name
          messageRouter: () => {
            throw new Error('Custom router error');
          },
          messageRoutingMode: 'CustomPartition',
        });

        // 4. Send 1000 messages in parallel for efficiency
        await expect(
          producer.send({ data: Buffer.from('test') }),
        ).rejects.toThrow('Custom router error');

        await producer.close();
      }, 30000);
    });
  });
})();
