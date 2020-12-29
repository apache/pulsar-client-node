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

#include "ProducerConfig.h"
#include <map>

static const std::string CFG_TOPIC = "topic";
static const std::string CFG_PRODUCER_NAME = "producerName";
static const std::string CFG_SEND_TIMEOUT = "sendTimeoutMs";
static const std::string CFG_INIT_SEQUENCE_ID = "initialSequenceId";
static const std::string CFG_MAX_PENDING = "maxPendingMessages";
static const std::string CFG_MAX_PENDING_ACROSS_PARTITIONS = "maxPendingMessagesAcrossPartitions";
static const std::string CFG_BLOCK_IF_QUEUE_FULL = "blockIfQueueFull";
static const std::string CFG_ROUTING_MODE = "messageRoutingMode";
static const std::string CFG_HASH_SCHEME = "hashingScheme";
static const std::string CFG_COMPRESS_TYPE = "compressionType";
static const std::string CFG_BATCH_ENABLED = "batchingEnabled";
static const std::string CFG_BATCH_MAX_DELAY = "batchingMaxPublishDelayMs";
static const std::string CFG_BATCH_MAX_MSG = "batchingMaxMessages";
static const std::string CFG_PROPS = "properties";
static const std::string CFG_PUBLIC_KEY_PATH = "publicKeyPath";
static const std::string CFG_PRIVATE_KEY_PATH = "privateKeyPath";
static const std::string CFG_ENCRYPTION_KEY = "encryptionKey";
static const std::string CFG_CRYPTO_FAILURE_ACTION = "cryptoFailureAction";

static const std::map<std::string, pulsar_partitions_routing_mode> MESSAGE_ROUTING_MODE = {
    {"UseSinglePartition", pulsar_UseSinglePartition},
    {"RoundRobinDistribution", pulsar_RoundRobinDistribution},
    {"CustomPartition", pulsar_CustomPartition}};

static const std::map<std::string, pulsar_hashing_scheme> HASHING_SCHEME = {
    {"Murmur3_32Hash", pulsar_Murmur3_32Hash},
    {"BoostHash", pulsar_BoostHash},
    {"JavaStringHash", pulsar_JavaStringHash},
};

static std::map<std::string, pulsar_compression_type> COMPRESSION_TYPE = {
    {"Zlib", pulsar_CompressionZLib},
    {"LZ4", pulsar_CompressionLZ4},
    {"ZSTD", pulsar_CompressionZSTD},
    {"SNAPPY", pulsar_CompressionSNAPPY},
};

static std::map<std::string, pulsar_producer_crypto_failure_action> PRODUCER_CRYPTO_FAILURE_ACTION = {
    {"FAIL", pulsar_ProducerFail},
    {"SEND", pulsar_ProducerSend},
};

ProducerConfig::ProducerConfig(const Napi::Object& producerConfig) : topic("") {
  this->cProducerConfig = pulsar_producer_configuration_create();

  if (producerConfig.Has(CFG_TOPIC) && producerConfig.Get(CFG_TOPIC).IsString()) {
    this->topic = producerConfig.Get(CFG_TOPIC).ToString().Utf8Value();
  }

  if (producerConfig.Has(CFG_PRODUCER_NAME) && producerConfig.Get(CFG_PRODUCER_NAME).IsString()) {
    std::string producerName = producerConfig.Get(CFG_PRODUCER_NAME).ToString().Utf8Value();
    if (!producerName.empty())
      pulsar_producer_configuration_set_producer_name(this->cProducerConfig, producerName.c_str());
  }

  if (producerConfig.Has(CFG_SEND_TIMEOUT) && producerConfig.Get(CFG_SEND_TIMEOUT).IsNumber()) {
    int32_t sendTimeoutMs = producerConfig.Get(CFG_SEND_TIMEOUT).ToNumber().Int32Value();
    if (sendTimeoutMs > 0) {
      pulsar_producer_configuration_set_send_timeout(this->cProducerConfig, sendTimeoutMs);
    }
  }

  if (producerConfig.Has(CFG_INIT_SEQUENCE_ID) && producerConfig.Get(CFG_INIT_SEQUENCE_ID).IsNumber()) {
    int64_t initialSequenceId = producerConfig.Get(CFG_INIT_SEQUENCE_ID).ToNumber().Int64Value();
    pulsar_producer_configuration_set_initial_sequence_id(this->cProducerConfig, initialSequenceId);
  }

  if (producerConfig.Has(CFG_MAX_PENDING) && producerConfig.Get(CFG_MAX_PENDING).IsNumber()) {
    int32_t maxPendingMessages = producerConfig.Get(CFG_MAX_PENDING).ToNumber().Int32Value();
    if (maxPendingMessages > 0) {
      pulsar_producer_configuration_set_max_pending_messages(this->cProducerConfig, maxPendingMessages);
    }
  }

  if (producerConfig.Has(CFG_MAX_PENDING_ACROSS_PARTITIONS) &&
      producerConfig.Get(CFG_MAX_PENDING_ACROSS_PARTITIONS).IsNumber()) {
    int32_t maxPendingMessagesAcrossPartitions =
        producerConfig.Get(CFG_MAX_PENDING_ACROSS_PARTITIONS).ToNumber().Int32Value();
    if (maxPendingMessagesAcrossPartitions > 0) {
      pulsar_producer_configuration_set_max_pending_messages(this->cProducerConfig,
                                                             maxPendingMessagesAcrossPartitions);
    }
  }

  if (producerConfig.Has(CFG_BLOCK_IF_QUEUE_FULL) &&
      producerConfig.Get(CFG_BLOCK_IF_QUEUE_FULL).IsBoolean()) {
    bool blockIfQueueFull = producerConfig.Get(CFG_BLOCK_IF_QUEUE_FULL).ToBoolean().Value();
    pulsar_producer_configuration_set_block_if_queue_full(this->cProducerConfig, blockIfQueueFull);
  }

  if (producerConfig.Has(CFG_ROUTING_MODE) && producerConfig.Get(CFG_ROUTING_MODE).IsString()) {
    std::string messageRoutingMode = producerConfig.Get(CFG_ROUTING_MODE).ToString().Utf8Value();
    if (MESSAGE_ROUTING_MODE.count(messageRoutingMode))
      pulsar_producer_configuration_set_partitions_routing_mode(this->cProducerConfig,
                                                                MESSAGE_ROUTING_MODE.at(messageRoutingMode));
  }

  if (producerConfig.Has(CFG_HASH_SCHEME) && producerConfig.Get(CFG_HASH_SCHEME).IsString()) {
    std::string hashingScheme = producerConfig.Get(CFG_HASH_SCHEME).ToString().Utf8Value();
    if (HASHING_SCHEME.count(hashingScheme))
      pulsar_producer_configuration_set_hashing_scheme(this->cProducerConfig,
                                                       HASHING_SCHEME.at(hashingScheme));
  }

  if (producerConfig.Has(CFG_COMPRESS_TYPE) && producerConfig.Get(CFG_COMPRESS_TYPE).IsString()) {
    std::string compressionType = producerConfig.Get(CFG_COMPRESS_TYPE).ToString().Utf8Value();
    if (COMPRESSION_TYPE.count(compressionType))
      pulsar_producer_configuration_set_compression_type(this->cProducerConfig,
                                                         COMPRESSION_TYPE.at(compressionType));
  }

  if (producerConfig.Has(CFG_BATCH_ENABLED) && producerConfig.Get(CFG_BATCH_ENABLED).IsBoolean()) {
    bool batchingEnabled = producerConfig.Get(CFG_BATCH_ENABLED).ToBoolean().Value();
    pulsar_producer_configuration_set_batching_enabled(this->cProducerConfig, batchingEnabled);
  }

  if (producerConfig.Has(CFG_BATCH_MAX_DELAY) && producerConfig.Get(CFG_BATCH_MAX_DELAY).IsNumber()) {
    int64_t batchingMaxPublishDelayMs = producerConfig.Get(CFG_BATCH_MAX_DELAY).ToNumber().Int64Value();
    if (batchingMaxPublishDelayMs > 0) {
      pulsar_producer_configuration_set_batching_max_publish_delay_ms(this->cProducerConfig,
                                                                      (long)batchingMaxPublishDelayMs);
    }
  }

  if (producerConfig.Has(CFG_BATCH_MAX_MSG) && producerConfig.Get(CFG_BATCH_MAX_MSG).IsNumber()) {
    uint32_t batchingMaxMessages = producerConfig.Get(CFG_BATCH_MAX_MSG).ToNumber().Uint32Value();
    if (batchingMaxMessages > 0) {
      pulsar_producer_configuration_set_batching_max_messages(this->cProducerConfig, batchingMaxMessages);
    }
  }

  if (producerConfig.Has(CFG_PROPS) && producerConfig.Get(CFG_PROPS).IsObject()) {
    Napi::Object propObj = producerConfig.Get(CFG_PROPS).ToObject();
    Napi::Array arr = propObj.GetPropertyNames();
    int size = arr.Length();
    for (int i = 0; i < size; i++) {
      Napi::String key = arr.Get(i).ToString();
      Napi::String value = propObj.Get(key).ToString();
      pulsar_producer_configuration_set_property(this->cProducerConfig, key.Utf8Value().c_str(),
                                                 value.Utf8Value().c_str());
    }
  }

  if ((producerConfig.Has(CFG_PUBLIC_KEY_PATH) && producerConfig.Has(CFG_PRIVATE_KEY_PATH)) &&
      (producerConfig.Get(CFG_PUBLIC_KEY_PATH).IsString() &&
       producerConfig.Get(CFG_PRIVATE_KEY_PATH).IsString())) {
    std::string publicKeyPath = producerConfig.Get(CFG_PUBLIC_KEY_PATH).ToString().Utf8Value();
    std::string privateKeyPath = producerConfig.Get(CFG_PRIVATE_KEY_PATH).ToString().Utf8Value();
    pulsar_producer_configuration_set_default_crypto_key_reader(this->cProducerConfig, publicKeyPath.c_str(),
                                                                privateKeyPath.c_str());
    if (producerConfig.Has(CFG_ENCRYPTION_KEY) && producerConfig.Get(CFG_ENCRYPTION_KEY).IsString()) {
      std::string encryptionKey = producerConfig.Get(CFG_ENCRYPTION_KEY).ToString().Utf8Value();
      pulsar_producer_configuration_set_encryption_key(this->cProducerConfig, encryptionKey.c_str());
    }
    if (producerConfig.Has(CFG_CRYPTO_FAILURE_ACTION) &&
        producerConfig.Get(CFG_CRYPTO_FAILURE_ACTION).IsString()) {
      std::string cryptoFailureAction = producerConfig.Get(CFG_CRYPTO_FAILURE_ACTION).ToString().Utf8Value();
      pulsar_producer_configuration_set_crypto_failure_action(
          this->cProducerConfig, PRODUCER_CRYPTO_FAILURE_ACTION.at(cryptoFailureAction));
    }
  }
}

ProducerConfig::~ProducerConfig() { pulsar_producer_configuration_free(this->cProducerConfig); }

pulsar_producer_configuration_t* ProducerConfig::GetCProducerConfig() { return this->cProducerConfig; }

std::string ProducerConfig::GetTopic() { return this->topic; }
