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

const httpUtils = require('./http_utils');
const Pulsar = require('../index');

const baseUrl = 'http://localhost:8080';

(() => {
  describe('Client', () => {
    describe('CreateFailedByUrlSetIncorrect', () => {
      test('No Set Url', async () => {
        const expectedError = 'Service URL is required and must be specified as a string '
          + 'unless serviceUrlProvider is configured';
        await expect(() => new Pulsar.Client({
          operationTimeoutSeconds: 30,
        })).toThrow(expectedError);
      });

      test('Set empty url', async () => {
        await expect(() => new Pulsar.Client({
          serviceUrl: '',
          operationTimeoutSeconds: 30,
        })).toThrow('Service URL is required and must be specified as a string');
      });

      test('Set invalid url', async () => {
        await expect(() => new Pulsar.Client({
          serviceUrl: 'invalid://localhost:6655',
          operationTimeoutSeconds: 30,
        })).toThrow('Invalid scheme: invalid');
      });

      test('Set not string url', async () => {
        await expect(() => new Pulsar.Client({
          serviceUrl: -1,
          operationTimeoutSeconds: 30,
        })).toThrow('Service URL is required and must be specified as a string');
      });

      test('Set both service url and service url provider', async () => {
        await expect(() => new Pulsar.Client({
          serviceUrl: 'pulsar://localhost:6650',
          serviceUrlProvider: {
            primary: 'pulsar://localhost:6650',
            secondary: ['pulsar://localhost:6651'],
          },
          operationTimeoutSeconds: 30,
        })).toThrow('Only one of serviceUrl or serviceUrlProvider can be configured');
      });
    });
    describe('test getPartitionsForTopic', () => {
      test('GetPartitions for empty topic', async () => {
        const client = new Pulsar.Client({
          serviceUrl: 'pulsar://localhost:6650',
          operationTimeoutSeconds: 30,
        });

        await expect(client.getPartitionsForTopic(''))
          .rejects.toThrow('Failed to GetPartitionsForTopic: InvalidTopicName');
        await client.close();
      });

      test('Client/getPartitionsForTopic', async () => {
        const client = new Pulsar.Client({
          serviceUrl: 'pulsar://localhost:6650',
          operationTimeoutSeconds: 30,
        });

        // test on nonPartitionedTopic
        const nonPartitionedTopicName = 'test-non-partitioned-topic';
        const nonPartitionedTopic = `persistent://public/default/${nonPartitionedTopicName}`;
        const nonPartitionedTopicAdminURL = `${baseUrl}/admin/v2/persistent/public/default/${nonPartitionedTopicName}`;
        const createNonPartitionedTopicRes = await httpUtils.request(
          nonPartitionedTopicAdminURL, {
            headers: {
              'Content-Type': 'application/json',
            },
            method: 'PUT',
          },
        );
        expect(createNonPartitionedTopicRes.statusCode).toBe(204);

        const nonPartitionedTopicList = await client.getPartitionsForTopic(nonPartitionedTopic);
        expect(nonPartitionedTopicList).toEqual([nonPartitionedTopic]);

        // test on partitioned with number
        const partitionedTopicName = 'test-partitioned-topic-1';
        const partitionedTopic = `persistent://public/default/${partitionedTopicName}`;
        const partitionedTopicAdminURL = `${baseUrl}/admin/v2/persistent/public/default/${partitionedTopicName}/partitions`;
        const createPartitionedTopicRes = await httpUtils.request(
          partitionedTopicAdminURL, {
            headers: {
              'Content-Type': 'text/plain',
            },
            data: 4,
            method: 'PUT',
          },
        );
        expect(createPartitionedTopicRes.statusCode).toBe(204);

        const partitionedTopicList = await client.getPartitionsForTopic(partitionedTopic);
        expect(partitionedTopicList).toEqual([
          'persistent://public/default/test-partitioned-topic-1-partition-0',
          'persistent://public/default/test-partitioned-topic-1-partition-1',
          'persistent://public/default/test-partitioned-topic-1-partition-2',
          'persistent://public/default/test-partitioned-topic-1-partition-3',
        ]);

        const deleteNonPartitionedTopicRes = await httpUtils.request(nonPartitionedTopicAdminURL, { method: 'DELETE' });
        expect(deleteNonPartitionedTopicRes.statusCode).toBe(204);
        const deletePartitionedTopicRes = await httpUtils.request(partitionedTopicAdminURL, { method: 'DELETE' });
        expect(deletePartitionedTopicRes.statusCode).toBe(204);

        await client.close();
      });
    });

    describe('test getSchemaInfo', () => {
      test('Client/getSchemaInfo', async () => {
        const client = new Pulsar.Client({
          serviceUrl: 'pulsar://localhost:6650',
          operationTimeoutSeconds: 30,
        });

        const definitionV1 = {
          type: 'record',
          name: 'User',
          fields: [{ name: 'name', type: 'string' }],
        };
        // Compatible evolution of definitionV1: adds a field with a default.
        const definitionV2 = {
          type: 'record',
          name: 'User',
          fields: [
            { name: 'name', type: 'string' },
            { name: 'age', type: 'int', default: 0 },
          ],
        };

        const topic = `persistent://public/default/get-schema-info-${Date.now()}`;
        const consumer = await client.subscribe({
          topic,
          subscription: 'sub1',
          schema: { schemaType: 'Avro', schema: JSON.stringify(definitionV1) },
        });
        const producerV1 = await client.createProducer({
          topic,
          schema: { schemaType: 'Avro', schema: JSON.stringify(definitionV1) },
        });
        const producerV2 = await client.createProducer({
          topic,
          schema: { schemaType: 'Avro', schema: JSON.stringify(definitionV2) },
        });
        // The payloads are not decoded here, only the schema version each message carries.
        await producerV1.send({ data: Buffer.from('v1') });
        await producerV2.send({ data: Buffer.from('v2') });
        const msgV1 = await consumer.receive();
        const msgV2 = await consumer.receive();
        consumer.acknowledge(msgV1);
        consumer.acknowledge(msgV2);

        const schemaV1 = await client.getSchemaInfo(topic, msgV1.getLongSchemaVersion());
        const schemaV2 = await client.getSchemaInfo(topic, msgV2.getLongSchemaVersion());
        expect(schemaV1.schemaType).toBe('Avro');
        expect(JSON.parse(schemaV1.schema.toString())).toEqual(definitionV1);
        expect(schemaV2.schemaType).toBe('Avro');
        expect(JSON.parse(schemaV2.schema.toString())).toEqual(definitionV2);

        await expect(client.getSchemaInfo(topic, 99))
          .rejects.toThrow('Failed to get schema info: ');

        await producerV1.close();
        await producerV2.close();
        await consumer.close();
        await client.close();
      });

      test('KeyValue schema', async () => {
        const client = new Pulsar.Client({
          serviceUrl: 'pulsar://localhost:6650',
          operationTimeoutSeconds: 30,
        });

        // A key schema longer than 127 bytes, so its length prefix is not valid UTF-8.
        const keyDefinition = {
          type: 'record',
          name: 'Key',
          fields: [{ name: 'id', type: 'string', doc: 'x'.repeat(150) }],
        };
        const valueDefinition = {
          type: 'record',
          name: 'Value',
          fields: [{ name: 'count', type: 'int' }],
        };
        const topicName = `get-schema-info-key-value-${Date.now()}`;
        const topic = `persistent://public/default/${topicName}`;
        // The admin API takes the key and value schemas as JSON and stores them in the binary
        // KeyValue layout: [key length][key schema][value length][value schema].
        const postSchemaRes = await httpUtils.request(`${baseUrl}/admin/v2/schemas/public/default/${topicName}/schema`, {
          headers: { 'Content-Type': 'application/json' },
          data: {
            type: 'KEY_VALUE',
            schema: JSON.stringify({ key: keyDefinition, value: valueDefinition }),
            properties: {
              'key.schema.type': 'AVRO',
              'key.schema.name': 'Key',
              'key.schema.properties': '{}',
              'value.schema.type': 'AVRO',
              'value.schema.name': 'Value',
              'value.schema.properties': '{}',
              'kv.encoding.type': 'SEPARATED',
            },
          },
          method: 'POST',
        });
        expect(postSchemaRes.statusCode).toBe(200);

        const schema = await client.getSchemaInfo(topic, 0);
        expect(schema.schemaType).toBe('KeyValue');
        const data = schema.schema;
        const keyLength = data.readInt32BE(0);
        const valueLength = data.readInt32BE(4 + keyLength);
        expect(keyLength).toBeGreaterThan(127);
        expect(data.length).toBe(8 + keyLength + valueLength);
        expect(JSON.parse(data.subarray(4, 4 + keyLength).toString())).toEqual(keyDefinition);
        expect(JSON.parse(data.subarray(8 + keyLength).toString())).toEqual(valueDefinition);

        // The fetched schema is accepted back as-is: the producer reuses version 0.
        const producer = await client.createProducer({ topic, schema });
        await expect(client.getSchemaInfo(topic, 1)).rejects.toThrow('Failed to get schema info: ');

        await producer.close();
        await client.close();
      });

      test('Schema properties', async () => {
        const client = new Pulsar.Client({
          serviceUrl: 'pulsar://localhost:6650',
          operationTimeoutSeconds: 30,
        });

        const topic = `persistent://public/default/get-schema-info-properties-${Date.now()}`;
        // JSON.parse creates an own "__proto__" key, which a plain assignment would not.
        const properties = JSON.parse('{"owner":"team-a","__proto__":"value"}');
        const producer = await client.createProducer({
          topic,
          schema: { schemaType: 'Json', schema: JSON.stringify({ type: 'string' }), properties },
        });

        const schema = await client.getSchemaInfo(topic, 0);
        expect(Object.entries(schema.properties).sort()).toEqual([
          ['__proto__', 'value'],
          ['owner', 'team-a'],
        ]);

        await producer.close();
        await client.close();
      });

      test('Invalid arguments', async () => {
        const client = new Pulsar.Client({
          serviceUrl: 'pulsar://localhost:6650',
          operationTimeoutSeconds: 30,
        });

        const topic = 'persistent://public/default/my-topic';
        const expectedError = 'getSchemaInfo expects a topic (string) and a schema version (non-negative integer)';
        expect(() => client.getSchemaInfo(topic)).toThrow(expectedError);
        [0.5, -1, NaN, Infinity, 2 ** 53, '0'].forEach((version) => {
          expect(() => client.getSchemaInfo(topic, version)).toThrow(expectedError);
        });

        await client.close();
      });
    });
  });
})();
