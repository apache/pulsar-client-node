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

#ifndef SCHEMA_INFO_H
#define SCHEMA_INFO_H

#include <napi.h>
#include <pulsar/c/producer_configuration.h>
#include <pulsar/c/consumer_configuration.h>

class SchemaInfo {
 public:
  SchemaInfo(const Napi::Object &schemaInfo);
  ~SchemaInfo();
  void SetProducerSchema(std::shared_ptr<pulsar_producer_configuration_t> cProducerConfiguration);
  void SetConsumerSchema(std::shared_ptr<pulsar_consumer_configuration_t> cConsumerConfiguration);

 private:
  pulsar_schema_type cSchemaType;
  std::string name;
  std::string schema;
  pulsar_string_map_t *cProperties;
};

#endif
