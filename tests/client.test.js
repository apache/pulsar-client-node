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

const http = require('http');
const Pulsar = require('../index.js');

const baseUrl = 'http://localhost:8080';
const requestAdminApi = (url, { headers, data = {}, method = 'PUT' }) => new Promise((resolve, reject) => {
  const req = http.request(url, {
    headers,
    method,
  }, (res) => {
    let responseBody = '';
    res.on('data', (chunk) => {
      responseBody += chunk;
    });
    res.on('end', () => {
      resolve(responseBody);
    });
  });

  req.on('error', (error) => {
    reject(error);
  });

  req.write(JSON.stringify(data));

  req.end();
});

(() => {
  describe('Client', () => {
    describe('CreateFailedByUrlSetIncorrect', () => {
      test('No Set Url', async () => {
        await expect(() => new Pulsar.Client({
          operationTimeoutSeconds: 30,
        })).toThrow('Service URL is required and must be specified as a string');
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
        await requestAdminApi(`${baseUrl}/admin/v2/persistent/public/default/${nonPartitionedTopicName}`, {
          headers: {
            'Content-Type': 'application/json',
          },
        });
        const nonPartitionedTopicList = await client.getPartitionsForTopic(nonPartitionedTopic);
        expect(nonPartitionedTopicList).toEqual([nonPartitionedTopic]);

        // test on partitioned with number
        const partitionedTopicName = 'test-partitioned-topic-1';
        const partitionedTopic = `persistent://public/default/${partitionedTopicName}`;
        await requestAdminApi(`${baseUrl}/admin/v2/persistent/public/default/${partitionedTopicName}/partitions`, {
          headers: {
            'Content-Type': 'text/plain',
          },
          data: '4',
        });
        const partitionedTopicList = await client.getPartitionsForTopic(partitionedTopic);
        expect(partitionedTopicList).toEqual([
          'persistent://public/default/test-partitioned-topic-1-partition-0',
          'persistent://public/default/test-partitioned-topic-1-partition-1',
          'persistent://public/default/test-partitioned-topic-1-partition-2',
          'persistent://public/default/test-partitioned-topic-1-partition-3',
        ]);

        await client.close();
      });
    });
  });
})();
