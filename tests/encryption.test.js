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
const fs = require('fs');
const Pulsar = require('../index');

class MyCryptoKeyReader extends Pulsar.CryptoKeyReader {
  constructor(publicKeys, privateKeys) {
    super();
    this.publicKeys = publicKeys;
    this.privateKeys = privateKeys;
  }

  getPublicKey(keyName, _metadata) {
    const keyPath = this.publicKeys[keyName];
    if (keyPath) {
      try {
        const key = fs.readFileSync(keyPath);
        return { key, _metadata };
      } catch (e) {
        return null;
      }
    }
    return null;
  }

  getPrivateKey(keyName, _metadata) {
    const keyPath = this.privateKeys[keyName];
    if (keyPath) {
      try {
        const key = fs.readFileSync(keyPath);
        return { key, _metadata };
      } catch (e) {
        return null;
      }
    }
    return null;
  }
}

(() => {
  describe('Encryption', () => {
    let client;
    const publicKeyPath = path.join(__dirname, 'certificate/public-key.client-rsa.pem');
    const privateKeyPath = path.join(__dirname, 'certificate/private-key.client-rsa.pem');

    beforeAll(() => {
      client = new Pulsar.Client({
        serviceUrl: 'pulsar://localhost:6650',
        operationTimeoutSeconds: 30,
      });
    });

    afterAll(async () => {
      await client.close();
    });

    test('End-to-End Encryption', async () => {
      const topic = `persistent://public/default/test-encryption-${Date.now()}`;

      const cryptoKeyReader = new MyCryptoKeyReader(
        { 'my-key': publicKeyPath },
        { 'my-key': privateKeyPath },
      );

      const producer = await client.createProducer({
        topic,
        encryptionKeys: ['my-key'],
        cryptoKeyReader,
        cryptoFailureAction: 'FAIL',
      });

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub-encryption',
        cryptoKeyReader,
        cryptoFailureAction: 'CONSUME',
        subscriptionInitialPosition: 'Earliest',
      });

      const msgContent = 'my-secret-message';
      await producer.send({
        data: Buffer.from(msgContent),
      });

      const msg = await consumer.receive();
      expect(msg.getData().toString()).toBe(msgContent);
      const encCtx = msg.getEncryptionContext();
      expect(encCtx).not.toBeNull();
      expect(encCtx.isDecryptionFailed).toBe(false);
      expect(encCtx.keys).toBeDefined();
      expect(encCtx.keys.length).toBeGreaterThan(0);
      expect(encCtx.keys[0].value).toBeInstanceOf(Buffer);
      expect(encCtx.param).toBeInstanceOf(Buffer);
      expect(encCtx.algorithm).toBe('');
      expect(encCtx.compressionType).toBe('None');
      expect(encCtx.uncompressedMessageSize).toBe(0);
      expect(encCtx.batchSize).toBe(1);

      await consumer.acknowledge(msg);
      await producer.close();
      await consumer.close();
    });

    test('End-to-End Encryption with Batching and Compression', async () => {
      const topic = `persistent://public/default/test-encryption-batch-compress-${Date.now()}`;

      const cryptoKeyReader = new MyCryptoKeyReader(
        { 'my-key': publicKeyPath },
        { 'my-key': privateKeyPath },
      );

      const producer = await client.createProducer({
        topic,
        encryptionKeys: ['my-key'],
        cryptoKeyReader,
        cryptoFailureAction: 'FAIL',
        batchingEnabled: true,
        batchingMaxMessages: 10,
        batchingMaxPublishDelayMs: 100,
        compressionType: 'Zlib',
      });

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub-encryption-batch-compress',
        cryptoKeyReader,
        cryptoFailureAction: 'CONSUME',
        subscriptionInitialPosition: 'Earliest',
      });

      const numMessages = 10;
      const sendPromises = [];
      for (let i = 0; i < numMessages; i += 1) {
        sendPromises.push(producer.send({
          data: Buffer.from(`message-${i}`),
        }));
      }
      await Promise.all(sendPromises);

      for (let i = 0; i < numMessages; i += 1) {
        const msg = await consumer.receive();
        expect(msg.getData().toString()).toBe(`message-${i}`);
        const encCtx = msg.getEncryptionContext();
        expect(encCtx).not.toBeNull();
        expect(encCtx.isDecryptionFailed).toBe(false);
        expect(encCtx.keys).toBeDefined();
        expect(encCtx.keys.length).toBeGreaterThan(0);
        expect(encCtx.keys[0].value).toBeInstanceOf(Buffer);
        expect(encCtx.param).toBeInstanceOf(Buffer);
        expect(encCtx.algorithm).toBe('');
        expect(encCtx.compressionType).toBe('Zlib');
        expect(encCtx.uncompressedMessageSize).toBeGreaterThan(0);
        expect(encCtx.batchSize).toBe(numMessages);

        await consumer.acknowledge(msg);
      }

      await producer.close();
      await consumer.close();
    });

    test('Decryption Failure', async () => {
      const topic = `persistent://public/default/test-decryption-failure-${Date.now()}`;

      const cryptoKeyReader = new MyCryptoKeyReader(
        { 'my-key': publicKeyPath },
        { 'my-key': privateKeyPath },
      );

      const producer = await client.createProducer({
        topic,
        encryptionKeys: ['my-key'],
        cryptoKeyReader,
        cryptoFailureAction: 'FAIL',
      });

      const consumer = await client.subscribe({
        topic,
        subscription: 'sub-decryption-failure',
        cryptoFailureAction: 'CONSUME',
        subscriptionInitialPosition: 'Earliest',
      });

      const msgContent = 'my-secret-message';
      await producer.send({
        data: Buffer.from(msgContent),
      });

      const msg = await consumer.receive();
      expect(msg.getData().toString()).not.toBe(msgContent);

      const encCtx = msg.getEncryptionContext();
      expect(encCtx).not.toBeNull();
      expect(encCtx.isDecryptionFailed).toBe(true);
      expect(encCtx.keys).toBeDefined();
      expect(encCtx.keys.length).toBeGreaterThan(0);
      expect(encCtx.keys[0].value).toBeInstanceOf(Buffer);
      expect(encCtx.param).toBeInstanceOf(Buffer);

      await consumer.acknowledge(msg);
      await producer.close();
      await consumer.close();
    });
  });
})();
