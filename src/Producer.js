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

/**
 * @typedef {import('../index').ProducerMessage} ProducerMessage
 * @typedef {import('../index').MessageId} MessageId
 * @typedef {import('../index').ProducerConfig} ProducerConfig
 * @typedef {import('../index').Client} PulsarClient
 * @typedef {import('../index').Producer} AddonProducer
 */

const PARTITION_PROP_KEY = '__partition__';
const CACHE_TTL_MS = 60 * 1000;

class Producer {
  /**
   * This class is a JavaScript wrapper around the C++ N-API Producer object.
   * It should not be instantiated by users directly.
   * @param {PulsarClient} client
   * @param {Producer.h} addonProducer - The native addon producer instance.
   * @param {ProducerConfig} config - The original producer configuration object.
   */
  constructor(client, addonProducer, config) {
    /** @private */
    this.client = client;
    /** @private */
    this.addonProducer = addonProducer;
    /** @private */
    this.producerConfig = config;
    /** @private */
    this.numPartitions = undefined;
    /** @private */
    this.partitionsCacheTimestamp = 0;
  }

  /**
   * Sends a message. If a custom message router was provided, it is called first
   * to determine the partition before passing the message to the C++ addon.
   * @param {ProducerMessage} message - The message object to send.
   * @returns {Promise<MessageId>} A promise that resolves with the MessageId of the sent message.
   */
  async send(message) {
    // 1. Create a shallow copy of the message parameter at the beginning.
    const finalMessage = { ...message };
    const config = this.producerConfig;

    // Check if custom routing mode is enabled
    if (config.messageRoutingMode === 'CustomPartition') {
      if (typeof config.messageRouter === 'function') {
        const numPartitions = await this.getNumPartitions();
        const topicMetadata = { numPartitions };
        const partitionIndex = config.messageRouter(finalMessage, topicMetadata);
        if (typeof partitionIndex === 'number' && partitionIndex >= 0) {
          if (!finalMessage.properties) {
            finalMessage.properties = {};
          }
          finalMessage.properties[PARTITION_PROP_KEY] = String(partitionIndex);
        }
      } else {
        throw new Error("Producer is configured with 'CustomPartition' routing mode, "
            + "but a 'messageRouter' function was not provided.");
      }
    }

    // 3. Pass the modified copy to the C++ addon.
    return this.addonProducer.send(finalMessage);
  }

  /**
   * Gets the number of partitions for the topic, using a cache with a TTL.
   * @private
   * @returns {Promise<number>}
   */
  async getNumPartitions() {
    const now = Date.now();
    // Check if cache is missing or expired
    if (this.numPartitions === undefined || now > this.partitionsCacheTimestamp + CACHE_TTL_MS) {
      const partitions = await this.client.getPartitionsForTopic(this.getTopic());
      this.numPartitions = partitions.length;
      this.partitionsCacheTimestamp = now;
    }
    return this.numPartitions;
  }

  /**
   * Flushes all the messages buffered in the client.
   * @returns {Promise<null>}
   */
  async flush() {
    return this.addonProducer.flush();
  }

  /**
   * Closes the producer.
   * @returns {Promise<null>}
   */
  async close() {
    return this.addonProducer.close();
  }

  /**
   * Gets the producer name.
   * @returns {string}
   */
  getProducerName() {
    return this.addonProducer.getProducerName();
  }

  /**
   * Gets the topic name.
   * @returns {string}
   */
  getTopic() {
    return this.addonProducer.getTopic();
  }

  /**
   * Checks if the producer is connected.
   * @returns {boolean}
   */
  isConnected() {
    return this.addonProducer.isConnected();
  }
}

module.exports = Producer;
