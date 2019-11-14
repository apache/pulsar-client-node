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
#include <pulsar/c/consumer_configuration.h>
#include <map>

static const std::string CFG_TOPIC = "topic";
static const std::string CFG_SUBSCRIPTION = "subscription";
static const std::string CFG_SUBSCRIPTION_TYPE = "subscriptionType";
static const std::string CFG_ACK_TIMEOUT = "ackTimeoutMs";
static const std::string CFG_NACK_REDELIVER_TIMEOUT = "nAckRedeliverTimeoutMs";
static const std::string CFG_RECV_QUEUE = "receiverQueueSize";
static const std::string CFG_RECV_QUEUE_ACROSS_PARTITIONS = "receiverQueueSizeAcrossPartitions";
static const std::string CFG_CONSUMER_NAME = "consumerName";
static const std::string CFG_PROPS = "properties";

static const std::map<std::string, pulsar_consumer_type> SUBSCRIPTION_TYPE = {
    {"Exclusive", pulsar_ConsumerExclusive},
    {"Shared", pulsar_ConsumerShared},
    {"Failover", pulsar_ConsumerFailover}};

ConsumerConfig::ConsumerConfig(const Napi::Object &consumerConfig)
    : topic(""), subscription(""), ackTimeoutMs(0), nAckRedeliverTimeoutMs(60000) {
  this->cConsumerConfig = pulsar_consumer_configuration_create();

  if (consumerConfig.Has(CFG_TOPIC) && consumerConfig.Get(CFG_TOPIC).IsString()) {
    this->topic = consumerConfig.Get(CFG_TOPIC).ToString().Utf8Value();
  }

  if (consumerConfig.Has(CFG_SUBSCRIPTION) && consumerConfig.Get(CFG_SUBSCRIPTION).IsString()) {
    this->subscription = consumerConfig.Get(CFG_SUBSCRIPTION).ToString().Utf8Value();
  }

  if (consumerConfig.Has(CFG_SUBSCRIPTION_TYPE) && consumerConfig.Get(CFG_SUBSCRIPTION_TYPE).IsString()) {
    std::string subscriptionType = consumerConfig.Get(CFG_SUBSCRIPTION_TYPE).ToString().Utf8Value();
    if (SUBSCRIPTION_TYPE.count(subscriptionType)) {
      pulsar_consumer_configuration_set_consumer_type(this->cConsumerConfig,
                                                      SUBSCRIPTION_TYPE.at(subscriptionType));
    }
  }

  if (consumerConfig.Has(CFG_CONSUMER_NAME) && consumerConfig.Get(CFG_CONSUMER_NAME).IsString()) {
    std::string consumerName = consumerConfig.Get(CFG_CONSUMER_NAME).ToString().Utf8Value();
    if (!consumerName.empty()) pulsar_consumer_set_consumer_name(this->cConsumerConfig, consumerName.c_str());
  }

  if (consumerConfig.Has(CFG_ACK_TIMEOUT) && consumerConfig.Get(CFG_ACK_TIMEOUT).IsNumber()) {
    this->ackTimeoutMs = consumerConfig.Get(CFG_ACK_TIMEOUT).ToNumber().Int64Value();
    if (this->ackTimeoutMs == 0 || this->ackTimeoutMs >= MIN_ACK_TIMEOUT_MILLIS) {
      pulsar_consumer_set_unacked_messages_timeout_ms(this->cConsumerConfig, this->ackTimeoutMs);
    }
  }

  if (consumerConfig.Has(CFG_NACK_REDELIVER_TIMEOUT) && consumerConfig.Get(CFG_NACK_REDELIVER_TIMEOUT).IsNumber()) {
    this->nAckRedeliverTimeoutMs = consumerConfig.Get(CFG_NACK_REDELIVER_TIMEOUT).ToNumber().Int64Value();
    if (this->nAckRedeliverTimeoutMs >= 0) {
      pulsar_configure_set_negative_ack_redelivery_delay_ms(this->cConsumerConfig, this->nAckRedeliverTimeoutMs);
    }
  }

  if (consumerConfig.Has(CFG_RECV_QUEUE) && consumerConfig.Get(CFG_RECV_QUEUE).IsNumber()) {
    int32_t receiverQueueSize = consumerConfig.Get(CFG_RECV_QUEUE).ToNumber().Int32Value();
    if (receiverQueueSize >= 0) {
      pulsar_consumer_configuration_set_receiver_queue_size(this->cConsumerConfig, receiverQueueSize);
    }
  }

  if (consumerConfig.Has(CFG_RECV_QUEUE_ACROSS_PARTITIONS) &&
      consumerConfig.Get(CFG_RECV_QUEUE_ACROSS_PARTITIONS).IsNumber()) {
    int32_t receiverQueueSizeAcrossPartitions =
        consumerConfig.Get(CFG_RECV_QUEUE_ACROSS_PARTITIONS).ToNumber().Int32Value();
    if (receiverQueueSizeAcrossPartitions >= 0) {
      pulsar_consumer_set_max_total_receiver_queue_size_across_partitions(this->cConsumerConfig,
                                                                          receiverQueueSizeAcrossPartitions);
    }
  }

  if (consumerConfig.Has(CFG_PROPS) && consumerConfig.Get(CFG_PROPS).IsObject()) {
    Napi::Object propObj = consumerConfig.Get(CFG_PROPS).ToObject();
    Napi::Array arr = propObj.GetPropertyNames();
    int size = arr.Length();
    for (int i = 0; i < size; i++) {
      std::string key = arr.Get(i).ToString().Utf8Value();
      std::string value = propObj.Get(key).ToString().Utf8Value();
      pulsar_consumer_configuration_set_property(this->cConsumerConfig, key.c_str(), value.c_str());
    }
  }
}

ConsumerConfig::~ConsumerConfig() { pulsar_consumer_configuration_free(this->cConsumerConfig); }

pulsar_consumer_configuration_t *ConsumerConfig::GetCConsumerConfig() { return this->cConsumerConfig; }

std::string ConsumerConfig::GetTopic() { return this->topic; }
std::string ConsumerConfig::GetSubscription() { return this->subscription; }
int64_t ConsumerConfig::GetAckTimeoutMs() { return this->ackTimeoutMs; }
int64_t ConsumerConfig::GetNAckRedeliverTimeoutMs() { return this->nAckRedeliverTimeoutMs; }
