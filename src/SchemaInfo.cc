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
#include "SchemaInfo.h"
#include <map>

static const std::string CFG_SCHEMA_TYPE = "schemaType";
static const std::string CFG_NAME = "name";
static const std::string CFG_SCHEMA = "schema";
static const std::string CFG_PROPS = "properties";

static const std::map<std::string, pulsar_schema_type> SCHEMA_TYPE = {{"None", pulsar_None},
                                                                      {"String", pulsar_String},
                                                                      {"Json", pulsar_Json},
                                                                      {"Protobuf", pulsar_Protobuf},
                                                                      {"Avro", pulsar_Avro},
                                                                      {"Boolean", pulsar_Boolean},
                                                                      {"Int8", pulsar_Int8},
                                                                      {"Int16", pulsar_Int16},
                                                                      {"Int32", pulsar_Int32},
                                                                      {"Int64", pulsar_Int64},
                                                                      {"Float32", pulsar_Float32},
                                                                      {"Float64", pulsar_Float64},
                                                                      {"KeyValue", pulsar_KeyValue},
                                                                      {"Bytes", pulsar_Bytes},
                                                                      {"AutoConsume", pulsar_AutoConsume},
                                                                      {"AutoPublish", pulsar_AutoPublish}};

SchemaInfo::SchemaInfo(const Napi::Object &schemaInfo) : cSchemaType(pulsar_Bytes), name("BYTES"), schema() {
  this->cProperties = pulsar_string_map_create();
  if (schemaInfo.Has(CFG_SCHEMA_TYPE) && schemaInfo.Get(CFG_SCHEMA_TYPE).IsString()) {
    this->name = schemaInfo.Get(CFG_SCHEMA_TYPE).ToString().Utf8Value();
    this->cSchemaType = SCHEMA_TYPE.at(schemaInfo.Get(CFG_SCHEMA_TYPE).ToString().Utf8Value());
  }
  if (schemaInfo.Has(CFG_NAME) && schemaInfo.Get(CFG_NAME).IsString()) {
    this->name = schemaInfo.Get(CFG_NAME).ToString().Utf8Value();
  }
  if (schemaInfo.Has(CFG_SCHEMA) && schemaInfo.Get(CFG_SCHEMA).IsString()) {
    this->schema = schemaInfo.Get(CFG_SCHEMA).ToString().Utf8Value();
  }
  if (schemaInfo.Has(CFG_PROPS) && schemaInfo.Get(CFG_PROPS).IsObject()) {
    Napi::Object propObj = schemaInfo.Get(CFG_PROPS).ToObject();
    Napi::Array arr = propObj.GetPropertyNames();
    int size = arr.Length();
    for (int i = 0; i < size; i++) {
      Napi::String key = arr.Get(i).ToString();
      Napi::String value = propObj.Get(key).ToString();
      pulsar_string_map_put(this->cProperties, key.Utf8Value().c_str(), value.Utf8Value().c_str());
    }
  }
}

void SchemaInfo::SetProducerSchema(std::shared_ptr<pulsar_producer_configuration_t> cProducerConfiguration) {
  pulsar_producer_configuration_set_schema_info(cProducerConfiguration.get(), this->cSchemaType,
                                                this->name.c_str(), this->schema.c_str(), this->cProperties);
}

void SchemaInfo::SetConsumerSchema(std::shared_ptr<pulsar_consumer_configuration_t> cConsumerConfiguration) {
  pulsar_consumer_configuration_set_schema_info(cConsumerConfiguration.get(), this->cSchemaType,
                                                this->name.c_str(), this->schema.c_str(), this->cProperties);
}

SchemaInfo::~SchemaInfo() { pulsar_string_map_free(this->cProperties); }
