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

#include <pulsar/c/consumer_configuration.h>
#include "MessageListener.h"

#define MIN_ACK_TIMEOUT_MILLIS 10000

class ConsumerConfig {
 public:
  ConsumerConfig(const Napi::Object &consumerConfig, pulsar_message_listener messageListener);
  ~ConsumerConfig();
  std::shared_ptr<pulsar_consumer_configuration_t> GetCConsumerConfig();
  std::string GetTopic();
  std::vector<std::string> GetTopics();
  std::string GetTopicsPattern();
  std::string GetSubscription();
  int64_t GetAckTimeoutMs();
  int64_t GetNAckRedeliverTimeoutMs();

  MessageListenerCallback *GetListenerCallback();

 private:
  std::shared_ptr<pulsar_consumer_configuration_t> cConsumerConfig;
  std::string topic;
  std::vector<std::string> topics;
  std::string topicsPattern;
  std::string subscription;
  int64_t ackTimeoutMs;
  int64_t nAckRedeliverTimeoutMs;
  MessageListenerCallback *listener;
};

#endif
