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

const disconnect = () => {
  if (process.connected) {
    process.disconnect();
  }
};

const fail = (error) => {
  console.error(error);
  process.exitCode = 1;
  disconnect();
};

const runScenario = async () => {
  if (typeof process.send !== 'function') {
    throw new Error('Scenario runner requires an IPC channel');
  }

  // eslint-disable-next-line global-require, import/no-dynamic-require
  const scenario = require(process.argv[2]);
  if (typeof scenario !== 'function') {
    throw new TypeError('Scenario module must export a function');
  }
  await scenario(...process.argv.slice(3));
};

runScenario().then(() => {
  process.send({ type: 'scenario-complete' }, (error) => {
    if (error) {
      fail(error);
      return;
    }
    disconnect();
  });
}).catch(fail);
