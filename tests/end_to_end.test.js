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
        'Failed to received message TimeOut',
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
        'Failed to received message TimeOut',
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
  });
})();
