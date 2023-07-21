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
const Pulsar = require('../index');
const httpRequest = require('./http_utils');

const baseUrl = 'http://localhost:8080';

(() => {
  describe('Reader', () => {
    test('No Topic', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      await expect(client.createReader({
        startMessageId: Pulsar.MessageId.earliest(),
      })).rejects.toThrow('Topic is required and must be specified as a string when creating reader');
      await client.close();
    });

    test('Not String Topic', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      await expect(client.createReader({
        topic: 0,
        startMessageId: Pulsar.MessageId.earliest(),
      })).rejects.toThrow('Topic is required and must be specified as a string when creating reader');
      await client.close();
    });

    test('No StartMessageId', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      await expect(client.createReader({
        topic: 'persistent://public/default/topic',
      })).rejects.toThrow('StartMessageId is required and must be specified as a MessageId object when creating reader');
      await client.close();
    });

    test('Not StartMessageId as MessageId', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
      await expect(client.createReader({
        topic: 'persistent://public/default/topic',
        startMessageId: 'not MessageId',
      })).rejects.toThrow('StartMessageId is required and must be specified as a MessageId object when creating reader');
      await client.close();
    });

    test('Reader by Partitioned Topic', async () => {
      const client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });

      // Create partitioned topic.
      const partitionedTopicName = 'test-reader-partitioned-topic';
      const partitionedTopic = `persistent://public/default/${partitionedTopicName}`;
      const partitionedTopicAdminURL = `${baseUrl}/admin/v2/persistent/public/default/${partitionedTopicName}/partitions`;
      const createPartitionedTopicRes = await httpRequest(
        partitionedTopicAdminURL, {
          headers: {
            'Content-Type': 'text/plain',
          },
          data: 4,
          method: 'PUT',
        },
      );
      expect(createPartitionedTopicRes.statusCode).toBe(204);

      const producer = await client.createProducer({
        topic: partitionedTopic,
        sendTimeoutMs: 30000,
        batchingEnabled: true,
      });
      expect(producer).not.toBeNull();

      const reader = await client.createReader({
        topic: partitionedTopic,
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
