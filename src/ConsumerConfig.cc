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
#include "Message.h"
#include <pulsar/c/consumer_configuration.h>
#include <pulsar/c/consumer.h>
#include <map>

static const std::string CFG_TOPIC = "topic";
static const std::string CFG_SUBSCRIPTION = "subscription";
static const std::string CFG_SUBSCRIPTION_TYPE = "subscriptionType";
static const std::string CFG_INIT_POSITION = "subscriptionInitialPosition";
static const std::string CFG_ACK_TIMEOUT = "ackTimeoutMs";
static const std::string CFG_NACK_REDELIVER_TIMEOUT = "nAckRedeliverTimeoutMs";
static const std::string CFG_RECV_QUEUE = "receiverQueueSize";
static const std::string CFG_RECV_QUEUE_ACROSS_PARTITIONS = "receiverQueueSizeAcrossPartitions";
static const std::string CFG_CONSUMER_NAME = "consumerName";
static const std::string CFG_PROPS = "properties";
static const std::string CFG_LISTENER = "listener";

static const std::map<std::string, pulsar_consumer_type> SUBSCRIPTION_TYPE = {
    {"Exclusive", pulsar_ConsumerExclusive},
    {"Shared", pulsar_ConsumerShared},
    {"KeyShared", pulsar_ConsumerKeyShared},
    {"Failover", pulsar_ConsumerFailover}};

static const std::map<std::string, initial_position> INIT_POSITION = {
    {"Latest", initial_position_latest}, {"Earliest", initial_position_earliest}};

struct MessageListenerProxyData {
  std::shared_ptr<CConsumerWrapper> consumerWrapper;
  pulsar_message_t *cMessage;

  MessageListenerProxyData(std::shared_ptr<CConsumerWrapper> consumerWrapper, pulsar_message_t *cMessage)
      : consumerWrapper(consumerWrapper), cMessage(cMessage) {}
};

void MessageListenerProxy(Napi::Env env, Napi::Function jsCallback, MessageListenerProxyData *data) {
  Napi::Object msg = Message::NewInstance({}, data->cMessage);
  Napi::Object consumerObj = Consumer::constructor.New({});
  Consumer *consumer = Consumer::Unwrap(consumerObj);
  consumer->SetCConsumer(std::move(data->consumerWrapper));
  delete data;
  jsCallback.Call({msg, consumerObj});
}

void MessageListener(pulsar_consumer_t *cConsumer, pulsar_message_t *cMessage, void *ctx) {
  ListenerCallback *listenerCallback = (ListenerCallback *)ctx;
  if (listenerCallback->callback.Acquire() != napi_ok) {
    return;
  };
  MessageListenerProxyData *dataPtr =
      new MessageListenerProxyData(listenerCallback->consumerWrapper, cMessage);
  listenerCallback->callback.BlockingCall(dataPtr, MessageListenerProxy);
  listenerCallback->callback.Release();
}

void FinalizeListenerCallback(Napi::Env env, ListenerCallback *cb, void *) { delete cb; }

ConsumerConfig::ConsumerConfig(const Napi::Object &consumerConfig,
                               std::shared_ptr<CConsumerWrapper> consumerWrapper)
    : topic(""), subscription(""), ackTimeoutMs(0), nAckRedeliverTimeoutMs(60000), listener(nullptr) {
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

  if (consumerConfig.Has(CFG_INIT_POSITION) && consumerConfig.Get(CFG_INIT_POSITION).IsString()) {
    std::string initPosition = consumerConfig.Get(CFG_INIT_POSITION).ToString().Utf8Value();
    if (INIT_POSITION.count(initPosition)) {
      pulsar_consumer_set_subscription_initial_position(this->cConsumerConfig,
                                                        INIT_POSITION.at(initPosition));
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

  if (consumerConfig.Has(CFG_NACK_REDELIVER_TIMEOUT) &&
      consumerConfig.Get(CFG_NACK_REDELIVER_TIMEOUT).IsNumber()) {
    this->nAckRedeliverTimeoutMs = consumerConfig.Get(CFG_NACK_REDELIVER_TIMEOUT).ToNumber().Int64Value();
    if (this->nAckRedeliverTimeoutMs >= 0) {
      pulsar_configure_set_negative_ack_redelivery_delay_ms(this->cConsumerConfig,
                                                            this->nAckRedeliverTimeoutMs);
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

  if (consumerConfig.Has(CFG_LISTENER) && consumerConfig.Get(CFG_LISTENER).IsFunction()) {
    this->listener = new ListenerCallback();
    this->listener->consumerWrapper = consumerWrapper;
    Napi::ThreadSafeFunction callback = Napi::ThreadSafeFunction::New(
        consumerConfig.Env(), consumerConfig.Get(CFG_LISTENER).As<Napi::Function>(), "Listener Callback", 1,
        1, (void *)NULL, FinalizeListenerCallback, listener);
    this->listener->callback = std::move(callback);
    pulsar_consumer_configuration_set_message_listener(this->cConsumerConfig, &MessageListener,
                                                       this->listener);
  }
}

ConsumerConfig::~ConsumerConfig() {
  pulsar_consumer_configuration_free(this->cConsumerConfig);
  if (this->listener) {
    this->listener->callback.Release();
  }
}

pulsar_consumer_configuration_t *ConsumerConfig::GetCConsumerConfig() { return this->cConsumerConfig; }

std::string ConsumerConfig::GetTopic() { return this->topic; }
std::string ConsumerConfig::GetSubscription() { return this->subscription; }
ListenerCallback *ConsumerConfig::GetListenerCallback() {
  ListenerCallback *cb = this->listener;
  this->listener = nullptr;
  return cb;
}

int64_t ConsumerConfig::GetAckTimeoutMs() { return this->ackTimeoutMs; }
int64_t ConsumerConfig::GetNAckRedeliverTimeoutMs() { return this->nAckRedeliverTimeoutMs; }

CConsumerWrapper::CConsumerWrapper() : cConsumer(nullptr) {}

CConsumerWrapper::~CConsumerWrapper() {
  if (this->cConsumer) {
    pulsar_consumer_free(this->cConsumer);
  }
}
