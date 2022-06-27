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

#ifndef PRODUCER_CONFIG_H
#define PRODUCER_CONFIG_H

#include <napi.h>
#include <pulsar/c/producer_configuration.h>

class ProducerConfig {
 public:
  ProducerConfig(const Napi::Object &producerConfig);
  ~ProducerConfig();
  std::shared_ptr<pulsar_producer_configuration_t> GetCProducerConfig();
  std::string GetTopic();

 private:
  std::shared_ptr<pulsar_producer_configuration_t> cProducerConfig;
  std::string topic;
};

#endif
