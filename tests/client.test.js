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

      test('Set not string url', async () => {
        await expect(() => new Pulsar.Client({
          serviceUrl: -1,
          operationTimeoutSeconds: 30,
        })).toThrow('Service URL is required and must be specified as a string');
      });
    });
  });
})();
