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
import Pulsar = require('./index');

// Maximum parameters
(async () => {
  const authTls: Pulsar.AuthenticationTls = new Pulsar.AuthenticationTls({
    certificatePath: '/path/to/cert.pem',
    privateKeyPath: '/path/to/key.pem',
  });

  const authAthenz1: Pulsar.AuthenticationAthenz = new Pulsar.AuthenticationAthenz('{}');

  const authAthenz2: Pulsar.AuthenticationAthenz = new Pulsar.AuthenticationAthenz({
    tenantDomain: 'athenz.tenant.domain',
    tenantService: 'service',
    providerDomain: 'athenz.provider.domain',
    privateKey: 'file:///path/to/private.key',
    ztsUrl: 'https://localhost:8443',
    keyId: '0',
    principalHeader: 'Athenz-Principal-Auth',
    roleHeader: 'Athenz-Role-Auth',
    tokenExpirationTime: '3600',
  });

  const authOauth2PrivateKey: Pulsar.AuthenticationOauth2 = new Pulsar.AuthenticationOauth2({
    type: "client_credentials",
    issuer_url: "issuer-url",
    private_key: "credentials-file-path",
    audience: "audience",
    scope: "your-scope",
  });

  const authOauth2ClientId: Pulsar.AuthenticationOauth2 = new Pulsar.AuthenticationOauth2({
    type: "client_credentials",
    issuer_url: "issuer-url",
    client_id: "client-id",
    client_secret: "client-secret",
    audience: "audience",
    scope: "scope"
  });

  const authToken: Pulsar.AuthenticationToken = new Pulsar.AuthenticationToken({
    token: 'foobar',
  });

  const client: Pulsar.Client = new Pulsar.Client({
    serviceUrl: 'pulsar://localhost:6650',
    authentication: authToken,
    operationTimeoutSeconds: 30,
    ioThreads: 4,
    messageListenerThreads: 4,
    concurrentLookupRequest: 100,
    useTls: false,
    tlsTrustCertsFilePath: '/path/to/ca-cert.pem',
    tlsValidateHostname: false,
    tlsAllowInsecureConnection: false,
    statsIntervalInSeconds: 60,
    log: (level: Pulsar.LogLevel, file: string, line: number, message: string) => {
      switch (level) {
        case Pulsar.LogLevel.DEBUG:
          console.log(`[DEBUG] ${message}`);
          break;
        case Pulsar.LogLevel.INFO:
          console.log(`[INFO] ${message}`);
          break;
        case Pulsar.LogLevel.WARN:
          console.log(`[WARN] ${message}`);
          break;
        case Pulsar.LogLevel.ERROR:
          console.log(`[ERROR] ${message}`);
          break;
      }
    },
  });

  const producer1: Pulsar.Producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
    producerName: 'producer1',
    sendTimeoutMs: 10000,
    initialSequenceId: 1,
    maxPendingMessages: 100,
    maxPendingMessagesAcrossPartitions: 1000,
    blockIfQueueFull: false,
    messageRoutingMode: 'UseSinglePartition',
    hashingScheme: 'Murmur3_32Hash',
    compressionType: 'Zlib',
    batchingEnabled: false,
    batchingMaxPublishDelayMs: 50,
    batchingMaxMessages: 100,
    properties: {
      key1: 'value1',
      key2: 'value2',
    },
    publicKeyPath: '/path/to/public.key',
    encryptionKey: 'encryption-key',
    cryptoFailureAction: 'FAIL',
  });

  const producer2: Pulsar.Producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
    messageRoutingMode: 'RoundRobinDistribution',
    hashingScheme: 'BoostHash',
    compressionType: 'LZ4',
    cryptoFailureAction: 'SEND',
  });

  const producer3: Pulsar.Producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
    messageRoutingMode: 'CustomPartition',
    hashingScheme: 'JavaStringHash',
    compressionType: 'ZSTD',
  });

  const producer4: Pulsar.Producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
    compressionType: 'SNAPPY',
  });

  const producer5: Pulsar.Producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
    schema: {
      name: 'my-schema',
      schemaType: "Json",
      schema: "my-json-schema",
      properties: {
        key1: 'value1',
        key2: 'value2'
      }
    }
  });

  const consumer1: Pulsar.Consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'my-sub1',
    subscriptionType: 'Exclusive',
    subscriptionInitialPosition: 'Latest',
    ackTimeoutMs: 3000,
    nAckRedeliverTimeoutMs: 5000,
    receiverQueueSize: 1000,
    receiverQueueSizeAcrossPartitions: 5000,
    consumerName: 'consumer1',
    properties: {
      key1: 'value1',
      key2: 'value2',
    },
    readCompacted: false,
    privateKeyPath: '/path/to/private.key',
    cryptoFailureAction: 'FAIL',
  });

  const consumer2: Pulsar.Consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'my-sub2',
    subscriptionType: 'Shared',
    subscriptionInitialPosition: 'Earliest',
    cryptoFailureAction: 'DISCARD',
  });

  const consumer3: Pulsar.Consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'my-sub3',
    subscriptionType: 'KeyShared',
    listener: (message: Pulsar.Message, consumer: Pulsar.Consumer) => {
    },
    cryptoFailureAction: 'CONSUME',
  });

  const consumer4: Pulsar.Consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'my-sub4',
    subscriptionType: 'Failover',
  });

  const consumer5: Pulsar.Consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'my-sub5',
    schema: {
      name: 'my-schema',
      schemaType: "Json",
      schema: "my-json-schema",
      properties: {
        key1: 'value1',
        key2: 'value2'
      }
    }
  });

  const reader1: Pulsar.Reader = await client.createReader({
    topic: 'persistent://public/default/my-topic',
    startMessageId: Pulsar.MessageId.latest(),
    receiverQueueSize: 1000,
    readerName: 'reader1',
    subscriptionRolePrefix: 'reader-',
    readCompacted: false,
  });

  const reader2: Pulsar.Reader = await client.createReader({
    topic: 'persistent://public/default/my-topic',
    startMessageId: Pulsar.MessageId.earliest(),
  });

  const reader3: Pulsar.Reader = await client.createReader({
    topic: 'persistent://public/default/my-topic',
    startMessageId: Pulsar.MessageId.earliest(),
    listener: (message: Pulsar.Message, reader: Pulsar.Reader) => {
    },
  });

  const reader4: Pulsar.Reader = await client.createReader({
    topic: 'persistent://public/default/my-topic',
    startMessageId: Pulsar.MessageId.earliest(),
    privateKeyPath: '/path/to/private.key',
  });

  const reader5: Pulsar.Reader = await client.createReader({
    topic: 'persistent://public/default/my-topic',
    startMessageId: Pulsar.MessageId.earliest(),
    privateKeyPath: '/path/to/private.key',
    cryptoFailureAction: 'CONSUME',
  });

  const producerName: string = producer1.getProducerName();
  const topicName1: string = producer1.getTopic();
  const producerIsConnected: boolean = producer1.isConnected();

  const messageId1: Pulsar.MessageId = await producer1.send({
    data: Buffer.from('my-message'),
    properties: {
      key1: 'value1',
      key2: 'value2',
    },
    eventTimestamp: Date.now(),
    sequenceId: 10,
    partitionKey: 'key1',
    orderingKey: 'orderingKey1',
    replicationClusters: [
      'cluster1',
      'cluster2',
    ],
    deliverAfter: 30000,
    deliverAt: Date.now() + 30000,
    disableReplication: false,
  });

  const messageIdString: string = messageId1.toString();
  const messageIdBuffer: Buffer = messageId1.serialize();
  const messageId2: Pulsar.MessageId = Pulsar.MessageId.deserialize(messageIdBuffer);

  const message1: Pulsar.Message = await consumer1.receive();
  const message2: Pulsar.Message = await consumer2.receive(1000);
  const consumerIsConnected: boolean = consumer1.isConnected();

  consumer1.negativeAcknowledge(message1);
  consumer1.negativeAcknowledgeId(messageId1);
  consumer1.acknowledge(message1);
  consumer1.acknowledgeId(messageId1);
  consumer1.acknowledgeCumulative(message1);
  consumer1.acknowledgeCumulativeId(messageId1);

  const topicName2: string = message1.getTopicName();
  const properties: { [key: string]: string } = message1.getProperties();
  const messageBuffer: Buffer = message1.getData();
  const messageId3: Pulsar.MessageId = message1.getMessageId();
  const publishTime: number = message1.getPublishTimestamp();
  const eventTime: number = message1.getEventTimestamp();
  const redeliveryCount: number = message1.getRedeliveryCount();
  const partitionKey: string = message1.getPartitionKey();

  const message3: Pulsar.Message = await reader1.readNext();
  const message4: Pulsar.Message = await reader2.readNext(1000);
  const hasNext: boolean = reader1.hasNext();
  const readerIsConnected: boolean = reader1.isConnected();

  await producer1.flush();
  await producer1.close();
  await producer2.close();
  await producer3.close();
  await producer4.close();
  await producer5.close();
  await consumer1.unsubscribe();
  await consumer2.close();
  await consumer3.close();
  await consumer4.close();
  await consumer5.close();
  await reader1.close();
  await reader2.close();
  await reader3.close();
  await reader4.close();
  await reader5.close();
  await client.close();
})();

// Minimal parameters
(async () => {
  const authAthenz: Pulsar.AuthenticationAthenz = new Pulsar.AuthenticationAthenz({
    tenantDomain: 'athenz.tenant.domain',
    tenantService: 'service',
    providerDomain: 'athenz.provider.domain',
    privateKey: 'file:///path/to/private.key',
    ztsUrl: 'https://localhost:8443',
  });

  const client: Pulsar.Client = new Pulsar.Client({
    serviceUrl: 'pulsar://localhost:6650',
  });

  const producer: Pulsar.Producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
  });

  const consumer1: Pulsar.Consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'my-sub1',
  });

  const consumer2: Pulsar.Consumer = await client.subscribe({
    topics: ['persistent://public/default/my-topic'],
    subscription: 'my-sub2',
  });

  const consumer3: Pulsar.Consumer = await client.subscribe({
    topicsPattern: 'persistent://public/default/my-topi[a-z]',
    subscription: 'my-sub3',
  });

  const reader: Pulsar.Reader = await client.createReader({
    topic: 'persistent://public/default/my-topic',
    startMessageId: Pulsar.MessageId.latest(),
  });

  const messageId: Pulsar.MessageId = await producer.send({
    data: Buffer.from('my-message'),
  });

  await producer.close();
  await consumer1.close();
  await consumer2.close();
  await consumer3.close();
  await reader.close();
  await client.close();
})();

// Missing required parameters
(async () => {
  // @ts-expect-error
  const authAthenz: Pulsar.AuthenticationAthenz = new Pulsar.AuthenticationAthenz({
  });

  // @ts-expect-error
  const client: Pulsar.Client = new Pulsar.Client({
  });

  // @ts-expect-error
  const producer: Pulsar.Producer = await client.createProducer({
  });

  // @ts-expect-error
  const consumer: Pulsar.Consumer = await client.subscribe({
  });

  // @ts-expect-error
  const reader1: Pulsar.Reader = await client.createReader({
    topic: 'persistent://public/default/my-topic',
  });

  // @ts-expect-error
  const reader2: Pulsar.Reader = await client.createReader({
    startMessageId: Pulsar.MessageId.latest(),
  });

  // @ts-expect-error
  const messageId: Pulsar.MessageId = await producer.send({
  });
})();
