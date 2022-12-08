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

const lodash = require('lodash');
const Pulsar = require('../index.js');

(() => {
  describe('End To End', () => {
    test('Produce/Consume', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-consume';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
        ackTimeoutMs: 10000,
      });

      expect(consumer).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        consumer.acknowledge(msg);
        results.push(msg.getData().toString());
      }
      expect(lodash.difference(messages, results)).toEqual([]);

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('negativeAcknowledge', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-consume';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
        ackTimeoutMs: 10000,
        nAckRedeliverTimeoutMs: 1000,
      });

      expect(consumer).not.toBeNull();

      const message = 'my-message';
      producer.send({
        data: Buffer.from(message),
      });
      await producer.flush();

      const results = [];
      const msg = await consumer.receive();
      results.push(msg.getData().toString());
      consumer.negativeAcknowledge(msg);

      const msg2 = await consumer.receive();
      results.push(msg2.getData().toString());
      consumer.acknowledge(msg2);

      await expect(consumer.receive(1000)).rejects.toThrow(
        'Failed to receive message: TimeOut',
      );

      expect(results).toEqual([message, message]);

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('getRedeliveryCount', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-consume';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic,
        subscriptionType: 'Shared',
        subscription: 'sub1',
        ackTimeoutMs: 10000,
        nAckRedeliverTimeoutMs: 100,
      });

      expect(consumer).not.toBeNull();

      const message = 'my-message';
      producer.send({
        data: Buffer.from(message),
      });
      await producer.flush();

      let redeliveryCount;
      let msg;
      for (let index = 0; index < 3; index += 1) {
        msg = await consumer.receive();
        redeliveryCount = msg.getRedeliveryCount();
        consumer.negativeAcknowledge(msg);
      }
      expect(redeliveryCount).toBe(2);
      consumer.acknowledge(msg);

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('Produce/Consume Listener', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-consume-listener';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      let finish;
      const results = [];
      const finishPromise = new Promise((resolve) => {
        finish = resolve;
      });

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
        ackTimeoutMs: 10000,
        listener: (message, messageConsumer) => {
          const data = message.getData().toString();
          results.push(data);
          messageConsumer.acknowledge(message);
          if (results.length === 10) finish();
        },
      });

      expect(consumer).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      await finishPromise;
      expect(lodash.difference(messages, results)).toEqual([]);

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('Produce/Read Listener', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-read-listener';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      let finish;
      const results = [];
      const finishPromise = new Promise((resolve) => {
        finish = resolve;
      });

      const reader = await client.createReader({
        topic,
        startMessageId: Pulsar.MessageId.latest(),
        listener: (message) => {
          const data = message.getData().toString();
          results.push(data);
          if (results.length === 10) finish();
        },
      });

      expect(reader).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      await finishPromise;
      expect(lodash.difference(messages, results)).toEqual([]);

      await producer.close();
      await reader.close();
      await client.close();
    });

    test('acknowledgeCumulative', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/acknowledgeCumulative';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
        ackTimeoutMs: 10000,
      });
      expect(consumer).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        if (i === 9) {
          consumer.acknowledgeCumulative(msg);
        }
      }

      await expect(consumer.receive(1000)).rejects.toThrow(
        'Failed to receive message: TimeOut',
      );

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('subscriptionInitialPosition', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/subscriptionInitialPosition';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: false,
      });
      expect(producer).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 2; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      const latestConsumer = await client.subscribe({
        topic,
        subscription: 'latestSub',
        subscriptionInitialPosition: 'Latest',
      });
      expect(latestConsumer).not.toBeNull();

      const earliestConsumer = await client.subscribe({
        topic,
        subscription: 'earliestSub',
        subscriptionInitialPosition: 'Earliest',
      });
      expect(earliestConsumer).not.toBeNull();

      for (let i = 2; i < 4; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      const latestResults = [];
      const earliestResults = [];
      for (let i = 0; i < 4; i += 1) {
        if (i < 2) {
          const latestMsg = await latestConsumer.receive(5000);
          latestConsumer.acknowledge(latestMsg);
          latestResults.push(latestMsg.getData().toString());
        }

        const earliestMsg = await earliestConsumer.receive(5000);
        earliestConsumer.acknowledge(earliestMsg);
        earliestResults.push(earliestMsg.getData().toString());
      }
      expect(lodash.difference(messages, latestResults)).toEqual(['my-message-0', 'my-message-1']);
      expect(lodash.difference(messages, earliestResults)).toEqual([]);

      await producer.close();
      await latestConsumer.close();
      await earliestConsumer.close();
      await client.close();
    });

    test('Produce/Read', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      expect(client).not.toBeNull();

      const topic = 'persistent://public/default/produce-read';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const reader = await client.createReader({
        topic,
        startMessageId: Pulsar.MessageId.latest(),
      });
      expect(reader).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      expect(reader.hasNext()).toBe(true);

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await reader.readNext();
        results.push(msg.getData().toString());
      }
      expect(lodash.difference(messages, results)).toEqual([]);

      expect(reader.hasNext()).toBe(false);

      await producer.close();
      await reader.close();
      await client.close();
    });

    test('Produce-Delayed/Consume', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      expect(client).not.toBeNull();

      const topic = 'persistent://public/default/produce-read-delayed';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub',
        subscriptionType: 'Shared',
      });
      expect(consumer).not.toBeNull();

      const messages = [];
      const time = (new Date()).getTime();
      for (let i = 0; i < 5; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
          deliverAfter: 3000,
        });
        messages.push(msg);
      }
      for (let i = 5; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
          deliverAt: (new Date()).getTime() + 3000,
        });
        messages.push(msg);
      }
      await producer.flush();

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        results.push(msg.getData().toString());
        consumer.acknowledge(msg);
      }
      expect(lodash.difference(messages, results)).toEqual([]);
      expect((new Date()).getTime() - time).toBeGreaterThan(3000);

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('Produce/Consume/Unsubscribe', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-consume-unsubscribe';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
        ackTimeoutMs: 10000,
      });

      expect(consumer).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        consumer.acknowledge(msg);
        results.push(msg.getData().toString());
      }
      expect(lodash.difference(messages, results)).toEqual([]);

      await consumer.unsubscribe();
      producer.send({ data: Buffer.from('drop') });
      await producer.flush();

      const consumer2 = await client.subscribe({
        topic,
        subscription: 'sub1',
        ackTimeoutMs: 10000,
      });

      const testData = 'success';
      producer.send({ data: Buffer.from(testData) });
      await producer.flush();

      const msg = await consumer2.receive();
      consumer2.acknowledge(msg);
      expect(msg.getData().toString()).toBe(testData);

      await consumer2.close();
      await producer.close();
      await client.close();
    });

    test('Produce/Read (Compression)', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      expect(client).not.toBeNull();

      const topic = 'persistent://public/default/produce-read-compression';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
        compressionType: 'ZSTD',
      });
      expect(producer).not.toBeNull();

      const reader = await client.createReader({
        topic,
        startMessageId: Pulsar.MessageId.latest(),
      });
      expect(reader).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      expect(reader.hasNext()).toBe(true);

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await reader.readNext();
        results.push(msg.getData().toString());
      }
      expect(lodash.difference(messages, results)).toEqual([]);

      expect(reader.hasNext()).toBe(false);

      await producer.close();
      await reader.close();
      await client.close();
    });

    test('Produce/Consume-Pattern', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      expect(client).not.toBeNull();

      const topic1 = 'persistent://public/default/produce-abcdef';
      const topic2 = 'persistent://public/default/produce-abczef';
      const topicsPattern = 'persistent://public/default/produce-abc[a-z]ef';
      const producer1 = await client.createProducer({
        topic: topic1,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer1).not.toBeNull();
      const producer2 = await client.createProducer({
        topic: topic2,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer2).not.toBeNull();

      const consumer = await client.subscribe({
        topicsPattern,
        subscription: 'sub',
        subscriptionType: 'Shared',
      });
      expect(consumer).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 5; i += 1) {
        const msg = `my-message-${i}`;
        producer1.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer1.flush();
      for (let i = 5; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer2.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer2.flush();

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        results.push(msg.getData().toString());
        consumer.acknowledge(msg);
      }
      expect(lodash.difference(messages, results)).toEqual([]);

      await producer1.close();
      await producer2.close();
      await consumer.close();
      await client.close();
    });

    test('Produce/Consume-Multi-Topic', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      expect(client).not.toBeNull();

      const topic1 = 'persistent://public/default/produce-mtopic-1';
      const topic2 = 'persistent://public/default/produce-mtopic-2';
      const producer1 = await client.createProducer({
        topic: topic1,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer1).not.toBeNull();
      const producer2 = await client.createProducer({
        topic: topic2,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer2).not.toBeNull();

      const consumer = await client.subscribe({
        topics: [topic1, topic2],
        subscription: 'sub',
        subscriptionType: 'Shared',
      });
      expect(consumer).not.toBeNull();

      const messages = [];
      for (let i = 0; i < 5; i += 1) {
        const msg = `my-message-${i}`;
        producer1.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer1.flush();
      for (let i = 5; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer2.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer2.flush();

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        results.push(msg.getData().toString());
        consumer.acknowledge(msg);
      }
      expect(lodash.difference(messages, results)).toEqual([]);

      await producer1.close();
      await producer2.close();
      await consumer.close();
      await client.close();
    });

    test('Produce/Consume and validate MessageId', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-consume-message-id';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
      });

      expect(consumer).not.toBeNull();

      const messages = [];
      const messageIds = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        const msgId = await producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
        messageIds.push(msgId.toString());
      }

      const results = [];
      const resultIds = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        consumer.acknowledge(msg);
        results.push(msg.getData().toString());
        resultIds.push(msg.getMessageId().toString());
      }
      expect(lodash.difference(messages, results)).toEqual([]);
      expect(lodash.difference(messageIds, resultIds)).toEqual([]);
      await producer.close();
      await consumer.close();
      await client.close();
    });
    test('Basic produce and consume encryption', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/encryption-produce-consume';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
        publicKeyPath: `${__dirname}/certificate/public-key.client-rsa.pem`,
        encryptionKey: 'encryption-key',
      });

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
        subscriptionType: 'Shared',
        ackTimeoutMs: 10000,
        privateKeyPath: `${__dirname}/certificate/private-key.client-rsa.pem`,
      });

      const messages = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        producer.send({
          data: Buffer.from(msg),
        });
        messages.push(msg);
      }
      await producer.flush();

      const results = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = await consumer.receive();
        consumer.acknowledge(msg);
        results.push(msg.getData().toString());
      }
      expect(lodash.difference(messages, results)).toEqual([]);
      await producer.close();
      await consumer.close();
      await client.close();
    });
    test('Produce/Consume/Read/IsConnected', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/produce-consume';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();
      expect(producer.isConnected()).toEqual(true);

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub1',
        ackTimeoutMs: 10000,
      });
      expect(consumer).not.toBeNull();
      expect(consumer.isConnected()).toEqual(true);

      const reader = await client.createReader({
        topic,
        startMessageId: Pulsar.MessageId.latest(),
      });
      expect(reader).not.toBeNull();
      expect(reader.isConnected()).toEqual(true);

      await producer.close();
      expect(producer.isConnected()).toEqual(false);

      await consumer.close();
      expect(consumer.isConnected()).toEqual(false);

      await reader.close();
      expect(reader.isConnected()).toEqual(false);

      await client.close();
    });

    test('Consumer seek by message Id', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/seek-by-msgid';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: false,
      });
      expect(producer).not.toBeNull();

      const msgIds = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        console.log(msg);
        const msgId = await producer.send({
          data: Buffer.from(msg),
        });
        msgIds.push(msgId);
      }

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub',
      });
      expect(consumer).not.toBeNull();

      await consumer.seek(msgIds[5]);
      const msg = consumer.receive(1000);
      console.log((await msg).getMessageId().toString());
      expect((await msg).getData().toString()).toBe('my-message-6');

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('Consumer seek by timestamp', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/seek-by-timestamp';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: false,
      });
      expect(producer).not.toBeNull();

      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        console.log(msg);
        await producer.send({
          data: Buffer.from(msg),
        });
      }

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub',
      });
      expect(consumer).not.toBeNull();

      const currentTime = Date.now();
      console.log(currentTime);

      await consumer.seekTimestamp(currentTime);

      console.log('End seek');

      await expect(consumer.receive(1000)).rejects.toThrow('Failed to receive message: TimeOut');

      await consumer.seekTimestamp(currentTime - 100000);

      const msg = consumer.receive(1000);
      console.log((await msg).getMessageId().toString());
      expect((await msg).getData().toString()).toBe('my-message-0');

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('Reader seek by message Id', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/reader-seek-by-msgid';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: false,
      });
      expect(producer).not.toBeNull();

      const msgIds = [];
      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        console.log(msg);
        const msgId = await producer.send({
          data: Buffer.from(msg),
        });
        msgIds.push(msgId);
      }

      const reader = await client.createReader({
        topic,
        startMessageId: Pulsar.MessageId.latest(),
      });
      expect(reader).not.toBeNull();

      await reader.seek(msgIds[5]);
      expect(reader.hasNext()).toBe(true);
      const msg = reader.readNext(1000);
      console.log((await msg).getMessageId().toString());
      expect((await msg).getData().toString()).toBe('my-message-6');

      await producer.close();
      await reader.close();
      await client.close();
    });

    test('Reader seek by timestamp', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/reader-seek-timestamp';
      const producer = await client.createProducer({
        topic,
        sendTimeoutMs: 30000,
        batchingEnabled: false,
      });
      expect(producer).not.toBeNull();

      for (let i = 0; i < 10; i += 1) {
        const msg = `my-message-${i}`;
        console.log(msg);
        await producer.send({
          data: Buffer.from(msg),
        });
      }

      const reader = await client.createReader({
        topic,
        startMessageId: Pulsar.MessageId.latest(),
      });
      expect(reader).not.toBeNull();

      const currentTime = Date.now();
      console.log(currentTime);

      await reader.seekTimestamp(currentTime);

      console.log('End seek');

      expect(reader.hasNext()).toBe(false);

      await reader.seekTimestamp(currentTime - 100000);
      console.log('Seek to previous time');

      expect(reader.hasNext()).toBe(true);
      const msg = reader.readNext(1000);
      console.log((await msg).getMessageId().toString());
      expect((await msg).getData().toString()).toBe('my-message-0');

      await producer.close();
      await reader.close();
      await client.close();
    });

    test('Message chunking', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const topic = 'persistent://public/default/message-chunking';
      const producer = await client.createProducer({
        topic,
        batchingEnabled: false,
        chunkingEnabled: true,
      });

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub',
        maxPendingChunkedMessage: 15,
        autoAckOldestChunkedMessageOnQueueFull: true,
      });

      const sendMsg = Buffer.alloc(10 * 1024 * 1024);

      await producer.send({
        data: sendMsg,
      });

      const receiveMsg = await consumer.receive(3000);
      expect(receiveMsg.getData().length).toBe(sendMsg.length);
      await producer.close();
      await consumer.close();
      await client.close();
    });
  });
})();
