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

const path = require('path');
const { spawn } = require('child_process');

const scenariosDir = path.join(__dirname, 'process_exit');
const runner = path.join(scenariosDir, 'runner.js');

const expectNaturalExit = (script, args) => new Promise((resolve, reject) => {
  const child = spawn(process.execPath, [runner, script, ...args], {
    stdio: ['ignore', 'pipe', 'pipe', 'ipc'],
  });
  let stdout = '';
  let stderr = '';
  let spawnError;
  let protocolFailure;
  let timedOut;
  let completed = false;
  let disconnectedAfterCompletion = false;
  let naturalExitTimer;

  const watchdogTimer = setTimeout(() => {
    timedOut = completed ? 'IPC disconnect' : 'scenario completion';
    child.kill('SIGKILL');
  }, 60000);

  child.stdout.on('data', (data) => {
    stdout += data.toString();
  });
  child.stderr.on('data', (data) => {
    stderr += data.toString();
  });
  child.on('message', (message) => {
    if (message && message.type === 'scenario-complete') {
      completed = true;
    }
  });
  child.on('disconnect', () => {
    if (!completed) {
      protocolFailure = 'IPC disconnected before scenario completion';
      child.kill('SIGKILL');
      return;
    }

    disconnectedAfterCompletion = true;
    clearTimeout(watchdogTimer);
    naturalExitTimer = setTimeout(() => {
      timedOut = 'natural exit after scenario completion and IPC disconnect';
      child.kill('SIGKILL');
    }, 10000);
  });
  child.on('error', (error) => {
    spawnError = error;
    clearTimeout(watchdogTimer);
    clearTimeout(naturalExitTimer);
  });
  child.on('close', (code, signal) => {
    clearTimeout(watchdogTimer);
    clearTimeout(naturalExitTimer);
    if (timedOut || spawnError || protocolFailure || !completed
      || !disconnectedAfterCompletion || code !== 0 || signal !== null) {
      reject(new Error(`Child did not exit naturally: ${JSON.stringify({
        timedOut,
        spawnError: spawnError && spawnError.message,
        protocolFailure,
        completed,
        disconnectedAfterCompletion,
        code,
        signal,
        stdout,
        stderr,
      })}`));
      return;
    }
    resolve();
  });
});

describe('AuthenticationToken supplier process exit', () => {
  const scenario = path.join(scenariosDir, 'token_supplier.js');

  test.each(['sync', 'async'])('%s supplier exits after closing a client', async (supplier) => {
    await expectNaturalExit(scenario, [supplier]);
  }, 75000);
});
