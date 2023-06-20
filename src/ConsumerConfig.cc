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

#include "ConsumerConfig.h"
#include "Consumer.h"
#include "SchemaInfo.h"
#include "Message.h"
#include <pulsar/c/consumer_configuration.h>
#include <pulsar/c/consumer.h>
#include <map>

static const std::string CFG_TOPIC = "topic";
static const std::string CFG_TOPICS = "topics";
static const std::string CFG_TOPICS_PATTERN = "topicsPattern";
static const std::string CFG_SUBSCRIPTION = "subscription";
static const std::string CFG_SUBSCRIPTION_TYPE = "subscriptionType";
static const std::string CFG_INIT_POSITION = "subscriptionInitialPosition";
static const std::string CFG_REGEX_SUBSCRIPTION_MODE = "regexSubscriptionMode";
static const std::string CFG_ACK_TIMEOUT = "ackTimeoutMs";
static const std::string CFG_NACK_REDELIVER_TIMEOUT = "nAckRedeliverTimeoutMs";
static const std::string CFG_RECV_QUEUE = "receiverQueueSize";
static const std::string CFG_RECV_QUEUE_ACROSS_PARTITIONS = "receiverQueueSizeAcrossPartitions";
static const std::string CFG_CONSUMER_NAME = "consumerName";
static const std::string CFG_PROPS = "properties";
static const std::string CFG_LISTENER = "listener";
static const std::string CFG_READ_COMPACTED = "readCompacted";
static const std::string CFG_SCHEMA = "schema";
static const std::string CFG_PRIVATE_KEY_PATH = "privateKeyPath";
static const std::string CFG_CRYPTO_FAILURE_ACTION = "cryptoFailureAction";
static const std::string CFG_MAX_PENDING_CHUNKED_MESSAGE = "maxPendingChunkedMessage";
static const std::string CFG_AUTO_ACK_OLDEST_CHUNKED_MESSAGE_ON_QUEUE_FULL =
    "autoAckOldestChunkedMessageOnQueueFull";
static const std::string CFG_BATCH_INDEX_ACK_ENABLED = "batchIndexAckEnabled";
static const std::string CFG_DEAD_LETTER_POLICY = "deadLetterPolicy";
static const std::string CFG_DLQ_POLICY_TOPIC = "deadLetterTopic";
static const std::string CFG_DLQ_POLICY_MAX_REDELIVER_COUNT = "maxRedeliverCount";
static const std::string CFG_DLQ_POLICY_INIT_SUB_NAME = "initialSubscriptionName";

static const std::map<std::string, pulsar_consumer_type> SUBSCRIPTION_TYPE = {
    {"Exclusive", pulsar_ConsumerExclusive},
    {"Shared", pulsar_ConsumerShared},
    {"KeyShared", pulsar_ConsumerKeyShared},
    {"Failover", pulsar_ConsumerFailover}};

static const std::map<std::string, pulsar_consumer_regex_subscription_mode> REGEX_SUBSCRIPTION_MODE = {
    {"PersistentOnly", pulsar_consumer_regex_sub_mode_PersistentOnly},
    {"NonPersistentOnly", pulsar_consumer_regex_sub_mode_NonPersistentOnly},
    {"AllTopics", pulsar_consumer_regex_sub_mode_AllTopics}};

static const std::map<std::string, initial_position> INIT_POSITION = {
    {"Latest", initial_position_latest}, {"Earliest", initial_position_earliest}};

static const std::map<std::string, pulsar_consumer_crypto_failure_action> CONSUMER_CRYPTO_FAILURE_ACTION = {
    {"FAIL", pulsar_ConsumerFail},
    {"DISCARD", pulsar_ConsumerDiscard},
    {"CONSUME", pulsar_ConsumerConsume},
};

void FinalizeListenerCallback(Napi::Env env, MessageListenerCallback *cb, void *) { delete cb; }

ConsumerConfig::ConsumerConfig(const Napi::Object &consumerConfig, pulsar_message_listener messageListener)
    : topic(""),
      topicsPattern(""),
      subscription(""),
      ackTimeoutMs(0),
      nAckRedeliverTimeoutMs(60000),
      listener(nullptr) {
  this->cConsumerConfig = std::shared_ptr<pulsar_consumer_configuration_t>(
      pulsar_consumer_configuration_create(), pulsar_consumer_configuration_free);

  if (consumerConfig.Has(CFG_TOPIC) && consumerConfig.Get(CFG_TOPIC).IsString()) {
    this->topic = consumerConfig.Get(CFG_TOPIC).ToString().Utf8Value();
  }

  if (consumerConfig.Has(CFG_TOPICS) && consumerConfig.Get(CFG_TOPICS).IsArray()) {
    auto arr = consumerConfig.Get(CFG_TOPICS).As<Napi::Array>();
    for (uint32_t i = 0; i < arr.Length(); i++) {
      if (arr.Get(i).IsString()) {
        this->topics.emplace_back(arr.Get(i).ToString().Utf8Value());
      }
    }
  }

  if (consumerConfig.Has(CFG_TOPICS_PATTERN) && consumerConfig.Get(CFG_TOPICS_PATTERN).IsString()) {
    this->topicsPattern = consumerConfig.Get(CFG_TOPICS_PATTERN).ToString().Utf8Value();
  }

  if (consumerConfig.Has(CFG_SUBSCRIPTION) && consumerConfig.Get(CFG_SUBSCRIPTION).IsString()) {
    this->subscription = consumerConfig.Get(CFG_SUBSCRIPTION).ToString().Utf8Value();
  }

  if (consumerConfig.Has(CFG_SUBSCRIPTION_TYPE) && consumerConfig.Get(CFG_SUBSCRIPTION_TYPE).IsString()) {
    std::string subscriptionType = consumerConfig.Get(CFG_SUBSCRIPTION_TYPE).ToString().Utf8Value();
    if (SUBSCRIPTION_TYPE.count(subscriptionType)) {
      pulsar_consumer_configuration_set_consumer_type(this->cConsumerConfig.get(),
                                                      SUBSCRIPTION_TYPE.at(subscriptionType));
    }
  }

  if (consumerConfig.Has(CFG_INIT_POSITION) && consumerConfig.Get(CFG_INIT_POSITION).IsString()) {
    std::string initPosition = consumerConfig.Get(CFG_INIT_POSITION).ToString().Utf8Value();
    if (INIT_POSITION.count(initPosition)) {
      pulsar_consumer_set_subscription_initial_position(this->cConsumerConfig.get(),
                                                        INIT_POSITION.at(initPosition));
    }
  }

  if (consumerConfig.Has(CFG_REGEX_SUBSCRIPTION_MODE) &&
      consumerConfig.Get(CFG_REGEX_SUBSCRIPTION_MODE).IsString()) {
    std::string regexSubscriptionMode =
        consumerConfig.Get(CFG_REGEX_SUBSCRIPTION_MODE).ToString().Utf8Value();
    if (REGEX_SUBSCRIPTION_MODE.count(regexSubscriptionMode)) {
      pulsar_consumer_configuration_set_regex_subscription_mode(
          this->cConsumerConfig.get(), REGEX_SUBSCRIPTION_MODE.at(regexSubscriptionMode));
    }
  }

  if (consumerConfig.Has(CFG_CONSUMER_NAME) && consumerConfig.Get(CFG_CONSUMER_NAME).IsString()) {
    std::string consumerName = consumerConfig.Get(CFG_CONSUMER_NAME).ToString().Utf8Value();
    if (!consumerName.empty())
      pulsar_consumer_set_consumer_name(this->cConsumerConfig.get(), consumerName.c_str());
  }

  if (consumerConfig.Has(CFG_ACK_TIMEOUT) && consumerConfig.Get(CFG_ACK_TIMEOUT).IsNumber()) {
    this->ackTimeoutMs = consumerConfig.Get(CFG_ACK_TIMEOUT).ToNumber().Int64Value();
    if (this->ackTimeoutMs == 0 || this->ackTimeoutMs >= MIN_ACK_TIMEOUT_MILLIS) {
      pulsar_consumer_set_unacked_messages_timeout_ms(this->cConsumerConfig.get(), this->ackTimeoutMs);
    }
  }

  if (consumerConfig.Has(CFG_NACK_REDELIVER_TIMEOUT) &&
      consumerConfig.Get(CFG_NACK_REDELIVER_TIMEOUT).IsNumber()) {
    this->nAckRedeliverTimeoutMs = consumerConfig.Get(CFG_NACK_REDELIVER_TIMEOUT).ToNumber().Int64Value();
    if (this->nAckRedeliverTimeoutMs >= 0) {
      pulsar_configure_set_negative_ack_redelivery_delay_ms(this->cConsumerConfig.get(),
                                                            this->nAckRedeliverTimeoutMs);
    }
  }

  if (consumerConfig.Has(CFG_RECV_QUEUE) && consumerConfig.Get(CFG_RECV_QUEUE).IsNumber()) {
    int32_t receiverQueueSize = consumerConfig.Get(CFG_RECV_QUEUE).ToNumber().Int32Value();
    if (receiverQueueSize >= 0) {
      pulsar_consumer_configuration_set_receiver_queue_size(this->cConsumerConfig.get(), receiverQueueSize);
    }
  }

  if (consumerConfig.Has(CFG_RECV_QUEUE_ACROSS_PARTITIONS) &&
      consumerConfig.Get(CFG_RECV_QUEUE_ACROSS_PARTITIONS).IsNumber()) {
    int32_t receiverQueueSizeAcrossPartitions =
        consumerConfig.Get(CFG_RECV_QUEUE_ACROSS_PARTITIONS).ToNumber().Int32Value();
    if (receiverQueueSizeAcrossPartitions >= 0) {
      pulsar_consumer_set_max_total_receiver_queue_size_across_partitions(this->cConsumerConfig.get(),
                                                                          receiverQueueSizeAcrossPartitions);
    }
  }

  if (consumerConfig.Has(CFG_SCHEMA) && consumerConfig.Get(CFG_SCHEMA).IsObject()) {
    SchemaInfo *schemaInfo = new SchemaInfo(consumerConfig.Get(CFG_SCHEMA).ToObject());
    schemaInfo->SetConsumerSchema(this->cConsumerConfig);
    delete schemaInfo;
  }

  if (consumerConfig.Has(CFG_PROPS) && consumerConfig.Get(CFG_PROPS).IsObject()) {
    Napi::Object propObj = consumerConfig.Get(CFG_PROPS).ToObject();
    Napi::Array arr = propObj.GetPropertyNames();
    int size = arr.Length();
    for (int i = 0; i < size; i++) {
      std::string key = arr.Get(i).ToString().Utf8Value();
      std::string value = propObj.Get(key).ToString().Utf8Value();
      pulsar_consumer_configuration_set_property(this->cConsumerConfig.get(), key.c_str(), value.c_str());
    }
  }

  if (consumerConfig.Has(CFG_LISTENER) && consumerConfig.Get(CFG_LISTENER).IsFunction()) {
    this->listener = new MessageListenerCallback();
    Napi::ThreadSafeFunction callback = Napi::ThreadSafeFunction::New(
        consumerConfig.Env(), consumerConfig.Get(CFG_LISTENER).As<Napi::Function>(), "Listener Callback", 1,
        1, (void *)NULL, FinalizeListenerCallback, listener);
    this->listener->callback = std::move(callback);
    pulsar_consumer_configuration_set_message_listener(this->cConsumerConfig.get(), messageListener,
                                                       this->listener);
  }

  if (consumerConfig.Has(CFG_READ_COMPACTED) && consumerConfig.Get(CFG_READ_COMPACTED).IsBoolean()) {
    bool readCompacted = consumerConfig.Get(CFG_READ_COMPACTED).ToBoolean();
    if (readCompacted) {
      pulsar_consumer_set_read_compacted(this->cConsumerConfig.get(), 1);
    }
  }

  if (consumerConfig.Has(CFG_PRIVATE_KEY_PATH) && consumerConfig.Get(CFG_PRIVATE_KEY_PATH).IsString()) {
    std::string publicKeyPath = "";
    std::string privateKeyPath = consumerConfig.Get(CFG_PRIVATE_KEY_PATH).ToString().Utf8Value();
    pulsar_consumer_configuration_set_default_crypto_key_reader(
        this->cConsumerConfig.get(), publicKeyPath.c_str(), privateKeyPath.c_str());
    if (consumerConfig.Has(CFG_CRYPTO_FAILURE_ACTION) &&
        consumerConfig.Get(CFG_CRYPTO_FAILURE_ACTION).IsString()) {
      std::string cryptoFailureAction = consumerConfig.Get(CFG_CRYPTO_FAILURE_ACTION).ToString().Utf8Value();
      if (CONSUMER_CRYPTO_FAILURE_ACTION.count(cryptoFailureAction)) {
        pulsar_consumer_configuration_set_crypto_failure_action(
            this->cConsumerConfig.get(), CONSUMER_CRYPTO_FAILURE_ACTION.at(cryptoFailureAction));
      }
    }
  }

  if (consumerConfig.Has(CFG_MAX_PENDING_CHUNKED_MESSAGE) &&
      consumerConfig.Get(CFG_MAX_PENDING_CHUNKED_MESSAGE).IsNumber()) {
    int32_t maxPendingChunkedMessage =
        consumerConfig.Get(CFG_MAX_PENDING_CHUNKED_MESSAGE).ToNumber().Int32Value();
    if (maxPendingChunkedMessage >= 0) {
      pulsar_consumer_configuration_set_max_pending_chunked_message(this->cConsumerConfig.get(),
                                                                    maxPendingChunkedMessage);
    }
  }

  if (consumerConfig.Has(CFG_AUTO_ACK_OLDEST_CHUNKED_MESSAGE_ON_QUEUE_FULL) &&
      consumerConfig.Get(CFG_AUTO_ACK_OLDEST_CHUNKED_MESSAGE_ON_QUEUE_FULL).IsBoolean()) {
    bool autoAckOldestChunkedMessageOnQueueFull =
        consumerConfig.Get(CFG_AUTO_ACK_OLDEST_CHUNKED_MESSAGE_ON_QUEUE_FULL).ToBoolean();
    pulsar_consumer_configuration_set_auto_ack_oldest_chunked_message_on_queue_full(
        this->cConsumerConfig.get(), autoAckOldestChunkedMessageOnQueueFull);
  }

  if (consumerConfig.Has(CFG_BATCH_INDEX_ACK_ENABLED) &&
      consumerConfig.Get(CFG_BATCH_INDEX_ACK_ENABLED).IsBoolean()) {
    bool batchIndexAckEnabled = consumerConfig.Get(CFG_BATCH_INDEX_ACK_ENABLED).ToBoolean();
    pulsar_consumer_configuration_set_batch_index_ack_enabled(this->cConsumerConfig.get(),
                                                              batchIndexAckEnabled);
  }

  if (consumerConfig.Has(CFG_DEAD_LETTER_POLICY) && consumerConfig.Get(CFG_DEAD_LETTER_POLICY).IsObject()) {
    pulsar_consumer_config_dead_letter_policy_t dlq_policy{};
    Napi::Object dlqPolicyObject = consumerConfig.Get(CFG_DEAD_LETTER_POLICY).ToObject();
    std::string dlq_topic_str;
    std::string init_subscription_name;
    if (dlqPolicyObject.Has(CFG_DLQ_POLICY_TOPIC) && dlqPolicyObject.Get(CFG_DLQ_POLICY_TOPIC).IsString()) {
      dlq_topic_str = dlqPolicyObject.Get(CFG_DLQ_POLICY_TOPIC).ToString().Utf8Value();
      dlq_policy.dead_letter_topic = dlq_topic_str.c_str();
    }
    if (dlqPolicyObject.Has(CFG_DLQ_POLICY_MAX_REDELIVER_COUNT) &&
        dlqPolicyObject.Get(CFG_DLQ_POLICY_MAX_REDELIVER_COUNT).IsNumber()) {
      dlq_policy.max_redeliver_count =
          dlqPolicyObject.Get(CFG_DLQ_POLICY_MAX_REDELIVER_COUNT).ToNumber().Int32Value();
    }
    if (dlqPolicyObject.Has(CFG_DLQ_POLICY_INIT_SUB_NAME) &&
        dlqPolicyObject.Get(CFG_DLQ_POLICY_INIT_SUB_NAME).IsString()) {
      init_subscription_name = dlqPolicyObject.Get(CFG_DLQ_POLICY_INIT_SUB_NAME).ToString().Utf8Value();
      dlq_policy.initial_subscription_name = init_subscription_name.c_str();
    }
    pulsar_consumer_configuration_set_dlq_policy(this->cConsumerConfig.get(), &dlq_policy);
  }
}

ConsumerConfig::~ConsumerConfig() {
  if (this->listener != nullptr) {
    this->listener->callback.Release();
  }
}

std::shared_ptr<pulsar_consumer_configuration_t> ConsumerConfig::GetCConsumerConfig() {
  return this->cConsumerConfig;
}

std::string ConsumerConfig::GetTopic() { return this->topic; }
std::vector<std::string> ConsumerConfig::GetTopics() { return this->topics; }
std::string ConsumerConfig::GetTopicsPattern() { return this->topicsPattern; }
std::string ConsumerConfig::GetSubscription() { return this->subscription; }
MessageListenerCallback *ConsumerConfig::GetListenerCallback() {
  MessageListenerCallback *cb = this->listener;
  this->listener = nullptr;
  return cb;
}

int64_t ConsumerConfig::GetAckTimeoutMs() { return this->ackTimeoutMs; }
int64_t ConsumerConfig::GetNAckRedeliverTimeoutMs() { return this->nAckRedeliverTimeoutMs; }
