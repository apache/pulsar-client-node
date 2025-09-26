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
/// <reference types="node" />

export interface ClientConfig {
  serviceUrl: string;
  authentication?: AuthenticationTls | AuthenticationAthenz | AuthenticationToken | AuthenticationOauth2 | AuthenticationBasic;
  operationTimeoutSeconds?: number;
  ioThreads?: number;
  messageListenerThreads?: number;
  concurrentLookupRequest?: number;
  useTls?: boolean;
  tlsTrustCertsFilePath?: string;
  tlsValidateHostname?: boolean;
  tlsAllowInsecureConnection?: boolean;
  statsIntervalInSeconds?: number;
  listenerName?: string;
  log?: (level: LogLevel, file: string, line: number, message: string) => void;
  logLevel?: LogLevel;
  connectionTimeoutMs?: number;
}

export class Client {
  static setLogHandler(logHandler: (level: LogLevel, file: string, line: number, message: string) => void): void;
  constructor(config: ClientConfig);
  createProducer(config: ProducerConfig): Promise<Producer>;
  subscribe(config: ConsumerConfig): Promise<Consumer>;
  createReader(config: ReaderConfig): Promise<Reader>;
  getPartitionsForTopic(topic: string): Promise<string[]>;
  close(): Promise<null>;
}

export interface ProducerConfig {
  topic: string;
  producerName?: string;
  sendTimeoutMs?: number;
  initialSequenceId?: number;
  maxPendingMessages?: number;
  maxPendingMessagesAcrossPartitions?: number;
  blockIfQueueFull?: boolean;
  messageRoutingMode?: MessageRoutingMode;
  hashingScheme?: HashingScheme;
  compressionType?: CompressionType;
  batchingEnabled?: boolean;
  batchingMaxPublishDelayMs?: number;
  batchingMaxMessages?: number;
  properties?: { [key: string]: string };
  publicKeyPath?: string;
  encryptionKey?: string;
  cryptoFailureAction?: ProducerCryptoFailureAction;
  chunkingEnabled?: boolean;
  schema?: SchemaInfo;
  accessMode?: ProducerAccessMode;
  batchingType?: ProducerBatchType;
  messageRouter?: MessageRouter;
}

export class Producer {
  send(message: ProducerMessage): Promise<MessageId>;
  flush(): Promise<null>;
  close(): Promise<null>;
  getProducerName(): string;
  getTopic(): string;
  isConnected(): boolean;
}

export interface ConsumerConfig {
  topic?: string;
  topics?: string[];
  topicsPattern?: string;
  subscription: string;
  subscriptionType?: SubscriptionType;
  subscriptionInitialPosition?: InitialPosition;
  ackTimeoutMs?: number;
  nAckRedeliverTimeoutMs?: number;
  receiverQueueSize?: number;
  receiverQueueSizeAcrossPartitions?: number;
  consumerName?: string;
  properties?: { [key: string]: string };
  listener?: (message: Message, consumer: Consumer) => void;
  readCompacted?: boolean;
  privateKeyPath?: string;
  cryptoFailureAction?: ConsumerCryptoFailureAction;
  maxPendingChunkedMessage?: number;
  autoAckOldestChunkedMessageOnQueueFull?: number;
  schema?: SchemaInfo;
  batchIndexAckEnabled?: boolean;
  regexSubscriptionMode?: RegexSubscriptionMode;
  deadLetterPolicy?: DeadLetterPolicy;
  batchReceivePolicy?: ConsumerBatchReceivePolicy;
  keySharedPolicy?: KeySharedPolicy;
}

export class Consumer {
  receive(timeout?: number): Promise<Message>;
  batchReceive(): Promise<Message []>;
  acknowledge(message: Message): Promise<null>;
  acknowledgeId(messageId: MessageId): Promise<null>;
  negativeAcknowledge(message: Message): void;
  negativeAcknowledgeId(messageId: MessageId): void;
  acknowledgeCumulative(message: Message): Promise<null>;
  acknowledgeCumulativeId(messageId: MessageId): Promise<null>;
  seek(messageId: MessageId): Promise<null>;
  seekTimestamp(timestamp: number): Promise<null>;
  isConnected(): boolean;
  close(): Promise<null>;
  unsubscribe(): Promise<null>;
}

export interface ReaderConfig {
  topic: string;
  startMessageId: MessageId;
  receiverQueueSize?: number;
  readerName?: string;
  subscriptionRolePrefix?: string;
  readCompacted?: boolean;
  listener?: (message: Message, reader: Reader) => void;
  privateKeyPath?: string;
  cryptoFailureAction?: ConsumerCryptoFailureAction;
}

export class Reader {
  readNext(timeout?: number): Promise<Message>;
  hasNext(): boolean;
  isConnected(): boolean;
  seek(messageId: MessageId): Promise<null>;
  seekTimestamp(timestamp: number): Promise<null>;
  close(): Promise<null>;
}

export interface ProducerMessage {
  data: Buffer;
  properties?: { [key: string]: string };
  eventTimestamp?: number;
  sequenceId?: number;
  partitionKey?: string;
  orderingKey?: string;
  replicationClusters?: string[];
  deliverAfter?: number;
  deliverAt?: number;
  disableReplication?: boolean;
}

export class Message {
  getTopicName(): string;
  getProperties(): { [key: string]: string };
  getData(): Buffer;
  getMessageId(): MessageId;
  getPublishTimestamp(): number;
  getEventTimestamp(): number;
  getRedeliveryCount(): number;
  getPartitionKey(): string;
  getOrderingKey(): string;
}

export class MessageId {
  static earliest(): MessageId;
  static latest(): MessageId;
  static deserialize(data: Buffer): MessageId;
  serialize(): Buffer;
  toString(): string;
}

export interface TopicMetadata {
  numPartitions: number;
}

/**
 * @callback MessageRouter
 * @description When producing messages to a partitioned topic, this router is used to select the
 * target partition for each message. The router only works when the `messageRoutingMode` is set to
 * `CustomPartition`. Please note that `getTopicName()` cannot be called on the `message`, otherwise
 * the behavior will be undefined because the topic is unknown before sending the message.
 * @param message The message to be routed.
 * @param topicMetadata Metadata for the partitioned topic the message is being routed to.
 * @returns {number} The index of the target partition (must be a number between 0 and
 * topicMetadata.numPartitions - 1).
 */
export type MessageRouter = (message: Message, topicMetadata: TopicMetadata) => number;

export interface SchemaInfo {
  schemaType: SchemaType;
  name?: string;
  schema?: string;
  properties?: Record<string, string>;
}

export interface DeadLetterPolicy {
  deadLetterTopic: string;
  maxRedeliverCount?: number;
  initialSubscriptionName?: string;
}

export interface ConsumerBatchReceivePolicy {
  maxNumMessages?: number;
  maxNumBytes?: number;
  timeoutMs?: number;
}

export interface ConsumerKeyShareStickyRange {
  start: number;
  end: number;
}
export type ConsumerKeyShareStickyRanges = ConsumerKeyShareStickyRange[];

export interface KeySharedPolicy {
  keyShareMode?: ConsumerKeyShareMode;
  allowOutOfOrderDelivery?: boolean;
  stickyRanges?: ConsumerKeyShareStickyRanges;
}

export class AuthenticationTls {
  constructor(params: { certificatePath: string, privateKeyPath: string });
}

export class AuthenticationAthenz {
  constructor(params: string | AthenzConfig | AthenzX509Config);
}

export interface AthenzConfig {
  tenantDomain: string;
  tenantService: string;
  providerDomain: string;
  privateKey: string;
  ztsUrl: string;
  keyId?: string;
  principalHeader?: string;
  roleHeader?: string;
  caCert?: string;
  /**
   * @deprecated
   */
  tokenExpirationTime?: string;
}

export class AthenzX509Config {
  providerDomain: string;
  privateKey: string;
  x509CertChain: string;
  ztsUrl: string;
  roleHeader?: string;
  caCert?: string;
}

export class AuthenticationToken {
  constructor(params: { token: string | (() => string) | (() => Promise<string>) });
}

export class AuthenticationOauth2 {
  constructor(params: {
    type: string;
    issuer_url: string;
    client_id?: string;
    client_secret?: string;
    private_key?: string;
    audience?: string;
    scope?: string;
  });
}

export class AuthenticationBasic {
  constructor(params: {
    username: string;
    password: string;
  });
}

export enum LogLevel {
  DEBUG = 0,
  INFO = 1,
  WARN = 2,
  ERROR = 3,
}

export type MessageRoutingMode =
  'UseSinglePartition' |
  'RoundRobinDistribution' |
  'CustomPartition';

export type HashingScheme =
  'Murmur3_32Hash' |
  'BoostHash' |
  'JavaStringHash';

export type CompressionType =
  'Zlib' |
  'LZ4' |
  'ZSTD' |
  'SNAPPY';

export type ProducerBatchType =
    'DefaultBatching' |
    'KeyBasedBatching';

export type ProducerCryptoFailureAction =
  'FAIL' |
  'SEND';

export type SubscriptionType =
  'Exclusive' |
  'Shared' |
  'KeyShared' |
  'Failover';

export type InitialPosition =
  'Latest' |
  'Earliest';

export type ConsumerCryptoFailureAction =
  'FAIL' |
  'DISCARD' |
  'CONSUME';

export type ConsumerKeyShareMode =
    'AutoSplit' |
    'Sticky';

export type RegexSubscriptionMode =
  'PersistentOnly' |
  'NonPersistentOnly' |
  'AllTopics';

export type SchemaType =
  'None' |
  'String' |
  'Json' |
  'Protobuf' |
  'Avro' |
  'Boolean' |
  'Int8' |
  'Int16' |
  'Int32' |
  'Int64' |
  'Float32' |
  'Float64' |
  'KeyValue' |
  'Bytes' |
  'AutoConsume' |
  'AutoPublish';

export type ProducerAccessMode =
    'Shared' |
    'Exclusive' |
    'WaitForExclusive' |
    'ExclusiveWithFencing';
