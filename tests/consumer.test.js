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

      test('Ack timeout less 10000', async () => {
        await expect(client.subscribe({
          topic: 'test-topic',
          subscription: 'sub1',
          subscriptionType: 'Shared',
          ackTimeoutMs: 100,
        })).rejects.toThrow('Ack timeout should be 0 or greater than or equal to 10000');
      });

      test('NAck timeout less 0', async () => {
        await expect(client.subscribe({
          topic: 'test-topic',
          subscription: 'sub1',
          subscriptionType: 'Shared',
          nAckRedeliverTimeoutMs: -1,
        })).rejects.toThrow('NAck timeout should be greater than or equal to zero');
      });

      test('Batch Receive Config Error', async () => {
        await expect(client.subscribe({
          topic: 'test-batch-receive-policy-error',
          subscription: 'sub1',
          subscriptionType: 'Shared',
          batchReceivePolicy: {
            maxNumMessages: -1,
            maxNumBytes: -1,
            timeoutMs: -1,
          },
        })).rejects.toThrow('At least one of maxNumMessages, maxNumBytes and timeoutMs must be specified.');
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

        await expect(consumer.close()).resolves.toEqual(null);
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

      test('Regex subscription', async () => {
        const topicName1 = 'persistent://public/default/regex-sub-1';
        const topicName2 = 'persistent://public/default/regex-sub-2';
        const topicName3 = 'non-persistent://public/default/regex-sub-3';
        const topicName4 = 'persistent://public/default/no-match-regex-sub-2';
        const producer1 = await client.createProducer({
          topic: topicName1,
        });
        const producer2 = await client.createProducer({
          topic: topicName2,
        });
        const producer3 = await client.createProducer({
          topic: topicName3,
        });
        const producer4 = await client.createProducer({
          topic: topicName4,
        });

        const consumer = await client.subscribe({
          topicsPattern: 'persistent://public/default/regex-sub.*',
          subscription: 'sub1',
          subscriptionType: 'Shared',
          regexSubscriptionMode: 'AllTopics',
        });

        const num = 10;
        for (let i = 0; i < num; i += 1) {
          const msg = `my-message-${i}`;
          await producer1.send({ data: Buffer.from(msg) });
          await producer2.send({ data: Buffer.from(msg) });
          await producer3.send({ data: Buffer.from(msg) });
          await producer4.send({ data: Buffer.from(msg) });
        }
        const results = [];
        for (let i = 0; i < 3 * num; i += 1) {
          const msg = await consumer.receive();
          results.push(msg.getData().toString());
        }
        expect(results.length).toEqual(3 * num);
        // assert no more msgs.
        await expect(consumer.receive(1000)).rejects.toThrow(
          'Failed to receive message: TimeOut',
        );
        await producer1.close();
        await producer2.close();
        await producer3.close();
        await producer4.close();
        await consumer.close();
      });

      test('Dead Letter topic', async () => {
        const topicName = 'test-dead_letter_topic';
        const dlqTopicName = 'test-dead_letter_topic_customize';
        const producer = await client.createProducer({
          topic: topicName,
        });

        const maxRedeliverCountNum = 3;
        const consumer = await client.subscribe({
          topic: topicName,
          subscription: 'sub-1',
          subscriptionType: 'Shared',
          deadLetterPolicy: {
            deadLetterTopic: dlqTopicName,
            maxRedeliverCount: maxRedeliverCountNum,
            initialSubscriptionName: 'init-sub-1-dlq',
          },
          nAckRedeliverTimeoutMs: 50,
        });

        // Send messages.
        const sendNum = 5;
        const messages = [];
        for (let i = 0; i < sendNum; i += 1) {
          const msg = `my-message-${i}`;
          await producer.send({ data: Buffer.from(msg) });
          messages.push(msg);
        }

        // Redelivery all messages maxRedeliverCountNum time.
        let results = [];
        for (let i = 1; i <= maxRedeliverCountNum * sendNum + sendNum; i += 1) {
          const msg = await consumer.receive();
          results.push(msg);
          if (i % sendNum === 0) {
            results.forEach((message) => {
              console.log(`Redeliver message ${message.getData().toString()} ${i} times ${message.getRedeliveryCount()} redeliver Count`);
              consumer.negativeAcknowledge(message);
            });
            results = [];
          }
        }
        // assert no more msgs.
        await expect(consumer.receive(100)).rejects.toThrow(
          'Failed to receive message: TimeOut',
        );

        const dlqConsumer = await client.subscribe({
          topic: dlqTopicName,
          subscription: 'sub-1',
        });
        const dlqResult = [];
        for (let i = 0; i < sendNum; i += 1) {
          const msg = await dlqConsumer.receive();
          dlqResult.push(msg.getData().toString());
        }
        expect(dlqResult).toEqual(messages);

        // assert no more msgs.
        await expect(dlqConsumer.receive(500)).rejects.toThrow(
          'Failed to receive message: TimeOut',
        );

        producer.close();
        consumer.close();
        dlqConsumer.close();
      });

      test('Batch Receive by maxNumberMessages', async () => {
        const topicName = 'batch-receive-test-topic';
        const producer = await client.createProducer({
          topic: topicName,
        });

        const consumer = await client.subscribe({
          topic: topicName,
          subscription: 'sub1',
          subscriptionType: 'Shared',
          batchReceivePolicy: {
            maxNumMessages: 10,
            maxNumBytes: -1,
            timeoutMs: 500,
          },
        });
        const num = 10;
        const messages = [];
        for (let i = 0; i < num; i += 1) {
          const msg = `my-message-${i}`;
          await producer.send({ data: Buffer.from(msg) });
          messages.push(msg);
        }

        const receiveMessages = await consumer.batchReceive();
        expect(receiveMessages.length).toEqual(num);
        const results = [];
        for (let i = 0; i < receiveMessages.length; i += 1) {
          const msg = receiveMessages[i];
          console.log(msg.getData().toString());
          results.push(msg.getData().toString());
        }
        expect(results).toEqual(messages);

        // assert no more msgs.
        expect(await consumer.batchReceive()).toEqual([]);

        await producer.close();
        await consumer.close();
      });

      test('Batch Receive by timeOutMs', async () => {
        const topicName = 'batch-receive-test-topic-timeout';
        const producer = await client.createProducer({
          topic: topicName,
        });

        const consumer = await client.subscribe({
          topic: topicName,
          subscription: 'sub1',
          subscriptionType: 'Shared',
          batchReceivePolicy: {
            maxNumMessages: 100,
            maxNumBytes: -1,
            timeoutMs: 500,
          },
        });
        // just send 10 message waite trigger timeout.
        const num = 10;
        const messages = [];
        for (let i = 0; i < num; i += 1) {
          const msg = `my-message-${i}`;
          await producer.send({ data: Buffer.from(msg) });
          messages.push(msg);
        }

        const receiveMessages = await consumer.batchReceive();
        expect(receiveMessages.length).toEqual(num);
        const results = [];
        for (let i = 0; i < receiveMessages.length; i += 1) {
          const msg = receiveMessages[i];
          console.log(msg.getData().toString());
          results.push(msg.getData().toString());
        }
        expect(results).toEqual(messages);

        // assert no more msgs.
        expect(await consumer.batchReceive()).toEqual([]);

        await producer.close();
        await consumer.close();
      });
      test('testStickyConsumer', async () => {
        const topicName = `KeySharedPolicyTest-sticky-consumer-${Date.now()}`;
        const subName = 'SubscriptionName';
        const numMessages = 1000;
        const numConsumers = 3;

        // Create producer with round-robin routing
        const producer = await client.createProducer({
          topic: topicName,
          batchingEnabled: false,
          messageRoutingMode: 'RoundRobinDistribution',
        });

        // Create 3 consumers with different hash ranges
        const consumers = [];
        const stickyRanges = [
          { start: 0, end: 1000 }, // let consumer 1 handle small range
          { start: 1001, end: 30000 },
          { start: 30001, end: 65535 },
        ];
        let i = 0;
        while (i < numConsumers) {
          const consumer = await client.subscribe({
            topic: topicName,
            subscription: subName,
            subscriptionType: 'KeyShared',
            keySharedPolicy: {
              keyShareMode: 'Sticky',
              stickyRanges: [stickyRanges[i]],
            },
          });
          consumers.push(consumer);
          i += 1;
        }

        // Send messages with random keys
        const keys = Array.from({ length: 300 }, (_, index) => index.toString());
        let msgIndex = 0;
        while (msgIndex < numMessages) {
          const key = keys[Math.floor(Math.random() * keys.length)];
          await producer.send({
            data: Buffer.from(msgIndex.toString()),
            partitionKey: key,
          });
          msgIndex += 1;
        }

        const assertKeyConsumerIndex = (keyToConsumer, key, expectedIndex) => {
          const actualIndex = keyToConsumer.get(key);
          expect(actualIndex).toBe(expectedIndex, `Key ${key} assigned to different consumer`);
        };

        // Verify message distribution
        const messagesPerConsumer = Array(numConsumers).fill(0);
        const keyToConsumer = new Map();
        let messagesReceived = 0;
        // eslint-disable-next-line no-restricted-syntax
        for (const [index, consumer] of consumers.entries()) {
          let msg;
          // eslint-disable-next-line no-constant-condition
          while (true) {
            try {
              msg = await consumer.receive(2000);
            } catch (err) {
              if (err.message.includes('TimeOut')) {
                break;
              } else {
                console.error('Receive error:', err);
              }
            }
            const key = msg.getPartitionKey() || msg.getOrderingKey();
            messagesPerConsumer[index] += 1;
            messagesReceived += 1;
            if (keyToConsumer.has(key)) {
              assertKeyConsumerIndex(keyToConsumer, key, index);
            } else {
              keyToConsumer.set(key, index);
            }
            await consumer.acknowledge(msg);
          }
        }
        expect(messagesReceived).toBe(numMessages);

        // Verify even distribution across consumers
        console.log('Messages per consumer:', messagesPerConsumer);
        // Consumer 0 are expected to receive a message count < 100
        expect(messagesPerConsumer[0]).toBeLessThan(100);
        // Consumer 1 and 2 are expected to receive a message count > 400
        expect(messagesPerConsumer[1]).toBeGreaterThan(400);
        expect(messagesPerConsumer[2]).toBeGreaterThan(400);
      }, 20000);
    });
  });
})();
