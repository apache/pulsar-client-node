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

const childProcess = require('child_process');
const fs = require('fs');
const http = require('http');
const os = require('os');
const path = require('path');
const { promisify } = require('util');
const Pulsar = require('../index');

const execFile = promisify(childProcess.execFile);
const dockerImage = process.env.PULSAR_TEST_IMAGE || 'apachepulsar/pulsar:latest';
const testRunId = `${Date.now()}-${process.pid}`;
const tempRoot = path.join(os.tmpdir(), `pulsar-node-failover-client-test-${testRunId}`);
const startedContainers = [];

jest.setTimeout(180000);

const delay = (ms) => new Promise((resolve) => setTimeout(resolve, ms));

const docker = async (args, options = {}) => {
  const { stdout } = await execFile('docker', args, {
    maxBuffer: 1024 * 1024,
    ...options,
  });
  return stdout.trim();
};

const waitForHttpOk = async (url, timeoutMs = 60000) => {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const request = http.get(url, (response) => {
          response.resume();
          if (response.statusCode >= 200 && response.statusCode < 500) {
            resolve();
          } else {
            reject(new Error(`Unexpected status code ${response.statusCode}`));
          }
        });
        request.on('error', reject);
        request.setTimeout(2000, () => {
          request.destroy(new Error('Timed out waiting for Pulsar HTTP endpoint'));
        });
      });
      return;
    } catch (e) {
      await delay(1000);
    }
  }

  throw new Error(`Timed out waiting for ${url}`);
};

const writeStandaloneConfig = (clusterName, webPort, brokerPort) => {
  const clusterDir = path.join(tempRoot, clusterName);
  fs.mkdirSync(clusterDir, { recursive: true });
  fs.chmodSync(tempRoot, 0o755);
  fs.chmodSync(clusterDir, 0o755);

  const sourceConfig = fs.readFileSync(path.join(__dirname, 'conf', 'standalone.conf'), 'utf8');
  const config = sourceConfig
    .replace(/^brokerServicePort=.*$/m, `brokerServicePort=${brokerPort}`)
    .replace(/^brokerServicePortTls=.*$/m, `brokerServicePortTls=${brokerPort + 100}`)
    .replace(/^webServicePort=.*$/m, `webServicePort=${webPort}`)
    .replace(/^webServicePortTls=.*$/m, `webServicePortTls=${webPort + 100}`)
    .replace(/^advertisedListeners=.*$/m, 'advertisedListeners=');

  const configPath = path.join(clusterDir, 'standalone.conf');
  fs.writeFileSync(configPath, config);
  fs.chmodSync(configPath, 0o644);

  ['server.crt', 'server.key'].forEach((fileName) => {
    const targetPath = path.join(clusterDir, fileName);
    fs.copyFileSync(path.join(__dirname, 'certificate', fileName), targetPath);
    fs.chmodSync(targetPath, 0o644);
  });

  return clusterDir;
};

const getStandaloneLogs = async (containerId) => {
  const { stdout, stderr } = await execFile('docker', [
    'exec',
    containerId,
    'bash',
    '-lc',
    'cat logs/pulsar-standalone-*.out logs/pulsar-standalone-*.log 2>/dev/null || true',
  ], { maxBuffer: 1024 * 1024 }).catch((e) => e);

  return `${stdout || ''}${stderr || ''}`.trim();
};

const startStandaloneCluster = async ({ clusterName, webPort, brokerPort }) => {
  const confDir = writeStandaloneConfig(clusterName, webPort, brokerPort);
  const containerId = await docker([
    'run',
    '-i',
    '-p',
    `${webPort}:${webPort}`,
    '-p',
    `${brokerPort}:${brokerPort}`,
    '--rm',
    '--detach',
    dockerImage,
    'sleep',
    '3600',
  ]);
  startedContainers.push(containerId);

  await docker(['cp', confDir, `${containerId}:/pulsar/test-conf`]);
  await docker([
    'exec',
    '-i',
    containerId,
    'env',
    'PULSAR_STANDALONE_CONF=test-conf/standalone.conf',
    'PULSAR_STANDALONE_USE_ZOOKEEPER=1',
    'bin/pulsar-daemon',
    'start',
    'standalone',
    '--no-functions-worker',
    '--no-stream-storage',
    '--bookkeeper-dir',
    `data/bookkeeper-${clusterName}`,
  ]);
  try {
    await waitForHttpOk(`http://localhost:${webPort}/metrics`);
  } catch (e) {
    const logs = await getStandaloneLogs(containerId);
    throw new Error(`${e.message}\nStandalone logs for ${clusterName}:\n${logs}`);
  }

  return { containerId, serviceUrl: `pulsar://localhost:${brokerPort}` };
};

const stopContainer = async (containerId) => {
  await docker(['kill', containerId]).catch(() => {});
};

const sendAndReceive = async (client, topic, payload) => {
  let consumer;
  let producer;

  try {
    producer = await client.createProducer({ topic });
    await producer.send({ data: Buffer.from(payload) });

    consumer = await client.subscribe({
      topic,
      subscription: `sub-${testRunId}-${Math.random().toString(36).slice(2)}`,
      subscriptionInitialPosition: 'Earliest',
    });

    const message = await consumer.receive(10000);
    expect(message.getData().toString()).toBe(payload);
    await consumer.acknowledge(message);
  } finally {
    if (producer) {
      await producer.close().catch(() => {});
    }
    if (consumer) {
      await consumer.close().catch(() => {});
    }
  }
};

const retryUntil = async (operation, timeoutMs = 60000) => {
  const deadline = Date.now() + timeoutMs;
  let lastError;

  while (Date.now() < deadline) {
    try {
      return await operation();
    } catch (e) {
      lastError = e;
      await delay(1000);
    }
  }

  throw lastError;
};

describe('failoverClientTest', () => {
  let primaryCluster;
  let secondaryCluster;
  let client;

  beforeAll(async () => {
    primaryCluster = await startStandaloneCluster({
      clusterName: 'primary',
      webPort: 18080,
      brokerPort: 16650,
    });
    secondaryCluster = await startStandaloneCluster({
      clusterName: 'secondary',
      webPort: 18081,
      brokerPort: 16651,
    });

    client = new Pulsar.Client({
      serviceUrlProvider: {
        primary: primaryCluster.serviceUrl,
        secondary: [secondaryCluster.serviceUrl],
        checkIntervalMs: 1000,
        failoverThreshold: 1,
        switchBackThreshold: 1,
      },
      operationTimeoutSeconds: 30,
      connectionTimeoutMs: 1000,
    });
  });

  afterAll(async () => {
    if (client) {
      await client.close().catch(() => {});
    }

    await Promise.all(startedContainers.map(stopContainer));
    fs.rmSync(tempRoot, { force: true, recursive: true });
  });

  test('continues producing and consuming after the primary cluster stops', async () => {
    expect(client).toBeDefined();

    await retryUntil(() => sendAndReceive(
      client,
      `persistent://public/default/failover-primary-${testRunId}`,
      'message-before-failover',
    ));

    await stopContainer(primaryCluster.containerId);

    await retryUntil(() => sendAndReceive(
      client,
      `persistent://public/default/failover-secondary-${testRunId}`,
      'message-after-failover',
    ));
  });
});
