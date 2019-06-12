/**

- Licensed to the Apache Software Foundation (ASF) under one
- or more contributor license agreements.  See the NOTICE file
- distributed with this work for additional information
- regarding copyright ownership.  The ASF licenses this file
- to you under the Apache License, Version 2.0 (the
- "License"); you may not use this file except in compliance
- with the License.  You may obtain a copy of the License at
  *
- http://www.apache.org/licenses/LICENSE-2.0
  *
- Unless required by applicable law or agreed to in writing,
- software distributed under the License is distributed on an
- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
- KIND, either express or implied.  See the License for the
- specific language governing permissions and limitations
- under the License.
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

      const producer = await client.createProducer({
        topic: 'persistent://public/default/test-end-to-end',
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic: 'persistent://public/default/test-end-to-end',
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

    test('acknowledgeCumulative', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      const producer = await client.createProducer({
        topic: 'persistent://public/default/acknowledgeCumulative',
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const consumer = await client.subscribe({
        topic: 'persistent://public/default/acknowledgeCumulative',
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

      await expect(consumer.receive(1000)).rejects.toThrow('Failed to received message TimeOut');

      await producer.close();
      await consumer.close();
      await client.close();
    });

    test('Produce/Read', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      expect(client).not.toBeNull();

      const producer = await client.createProducer({
        topic: 'persistent://public/default/test-end-to-end',
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const reader = await client.createReader({
        topic: 'persistent://public/default/test-end-to-end',
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
