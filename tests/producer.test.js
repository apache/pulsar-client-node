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
        })).rejects.toThrow('Failed to create producer: BrokerMetadataError');
      });

      test('Not Exist Namespace', async () => {
        await expect(client.createProducer({
          topic: 'persistent://public/no-namespace/topic',
          sendTimeoutMs: 30000,
          batchingEnabled: true,
        })).rejects.toThrow('Failed to create producer: BrokerMetadataError');
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
  });
})();
