/// <reference types="node" />

export interface ClientConfig {
  serviceUrl: string;
  authentication?: AuthenticationTls | AuthenticationAthenz | AuthenticationToken;
  operationTimeoutSeconds?: number;
  ioThreads?: number;
  messageListenerThreads?: number;
  concurrentLookupRequest?: number;
  useTls?: boolean;
  tlsTrustCertsFilePath?: string;
  tlsValidateHostname?: boolean;
  tlsAllowInsecureConnection?: boolean;
  statsIntervalInSeconds?: number;
  log?: (level: LogLevel, file: string, line: number, message: string) => void;
}

export class Client {
  constructor(config: ClientConfig);
  createProducer(config: ProducerConfig): Promise<Producer>;
  subscribe(config: ConsumerConfig): Promise<Consumer>;
  createReader(config: ReaderConfig): Promise<Reader>;
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
}

export class Producer {
  send(message: ProducerMessage): Promise<MessageId>;
  flush(): Promise<null>;
  close(): Promise<null>;
  getProducerName(): string;
  getTopic(): string;
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
}

export class Consumer {
  receive(timeout?: number): Promise<Message>;
  acknowledge(message: Message): void;
  acknowledgeId(messageId: MessageId): void;
  negativeAcknowledge(message: Message): void;
  negativeAcknowledgeId(messageId: MessageId): void;
  acknowledgeCumulative(message: Message): void;
  acknowledgeCumulativeId(messageId: MessageId): void;
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
}

export class Reader {
  readNext(timeout?: number): Promise<Message>;
  hasNext(): boolean;
  close(): Promise<null>;
}

export interface ProducerMessage {
  data: Buffer;
  properties?: { [key: string]: string };
  eventTimestamp?: number;
  sequenceId?: number;
  partitionKey?: string;
  replicationClusters?: string[];
  deliverAfter?: number;
  deliverAt?: number;
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
}

export class MessageId {
  static earliest(): MessageId;
  static latest(): MessageId;
  static deserialize(data: Buffer): MessageId;
  serialize(): Buffer;
  toString(): string;
}

export class AuthenticationTls {
  constructor(params: { certificatePath: string, privateKeyPath: string });
}

export class AuthenticationAthenz {
  constructor(params: string);
}

export class AuthenticationToken {
  constructor(params: { token: string });
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

export type SubscriptionType =
  'Exclusive' |
  'Shared' |
  'KeyShared' |
  'Failover';

export type InitialPosition =
  'Latest' |
  'Earliest';
