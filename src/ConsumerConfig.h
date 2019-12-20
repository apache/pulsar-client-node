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

#ifndef CONSUMER_CONFIG_H
#define CONSUMER_CONFIG_H

#include <napi.h>
#include <pulsar/c/consumer_configuration.h>

#define MIN_ACK_TIMEOUT_MILLIS 10000

struct CConsumerWrapper {
  pulsar_consumer_t *cConsumer;
  CConsumerWrapper();
  ~CConsumerWrapper();
};

struct ListenerCallback {
  Napi::ThreadSafeFunction callback;
  std::shared_ptr<CConsumerWrapper> consumerWrapper;
};

class ConsumerConfig {
 public:
  ConsumerConfig(const Napi::Object &consumerConfig, std::shared_ptr<CConsumerWrapper> consumerWrapper);
  ~ConsumerConfig();
  pulsar_consumer_configuration_t *GetCConsumerConfig();
  std::string GetTopic();
  std::string GetSubscription();
  int64_t GetAckTimeoutMs();
  int64_t GetNAckRedeliverTimeoutMs();
  ListenerCallback *GetListenerCallback();

 private:
  pulsar_consumer_configuration_t *cConsumerConfig;
  std::string topic;
  std::string subscription;
  int64_t ackTimeoutMs;
  int64_t nAckRedeliverTimeoutMs;
  ListenerCallback *listener;
};

#endif
