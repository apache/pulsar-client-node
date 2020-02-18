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

#include "ReaderConfig.h"
#include "MessageId.h"
#include <map>

static const std::string CFG_TOPIC = "topic";
static const std::string CFG_START_MESSAGE_ID = "startMessageId";
static const std::string CFG_RECV_QUEUE = "receiverQueueSize";
static const std::string CFG_READER_NAME = "readerName";
static const std::string CFG_SUBSCRIPTION_ROLE_PREFIX = "subscriptionRolePrefix";
static const std::string CFG_READ_COMPACTED = "readCompacted";

ReaderConfig::ReaderConfig(const Napi::Object &readerConfig) : topic(""), cStartMessageId(NULL) {
  this->cReaderConfig = pulsar_reader_configuration_create();

  if (readerConfig.Has(CFG_TOPIC) && readerConfig.Get(CFG_TOPIC).IsString()) {
    this->topic = readerConfig.Get(CFG_TOPIC).ToString().Utf8Value();
  }
  if (readerConfig.Has(CFG_START_MESSAGE_ID) && readerConfig.Get(CFG_START_MESSAGE_ID).IsObject()) {
    Napi::Object objMessageId = readerConfig.Get(CFG_START_MESSAGE_ID).ToObject();
    MessageId *msgId = MessageId::Unwrap(objMessageId);
    this->cStartMessageId = msgId->GetCMessageId();
  }

  if (readerConfig.Has(CFG_RECV_QUEUE) && readerConfig.Get(CFG_RECV_QUEUE).IsNumber()) {
    int32_t receiverQueueSize = readerConfig.Get(CFG_RECV_QUEUE).ToNumber().Int32Value();
    if (receiverQueueSize >= 0) {
      pulsar_reader_configuration_set_receiver_queue_size(this->cReaderConfig, receiverQueueSize);
    }
  }

  if (readerConfig.Has(CFG_READER_NAME) && readerConfig.Get(CFG_READER_NAME).IsString()) {
    std::string readerName = readerConfig.Get(CFG_READER_NAME).ToString().Utf8Value();
    if (!readerName.empty())
      pulsar_reader_configuration_set_reader_name(this->cReaderConfig, readerName.c_str());
  }

  if (readerConfig.Has(CFG_SUBSCRIPTION_ROLE_PREFIX) &&
      readerConfig.Get(CFG_SUBSCRIPTION_ROLE_PREFIX).IsString()) {
    std::string subscriptionRolePrefix =
        readerConfig.Get(CFG_SUBSCRIPTION_ROLE_PREFIX).ToString().Utf8Value();
    if (!subscriptionRolePrefix.empty())
      pulsar_reader_configuration_set_reader_name(this->cReaderConfig, subscriptionRolePrefix.c_str());
  }

  if (readerConfig.Has(CFG_READ_COMPACTED) && readerConfig.Get(CFG_READ_COMPACTED).IsBoolean()) {
    bool readCompacted = readerConfig.Get(CFG_READ_COMPACTED).ToBoolean();
    if (readCompacted) {
      pulsar_reader_configuration_set_read_compacted(this->cReaderConfig, 1);
    }
  }
}

ReaderConfig::~ReaderConfig() { pulsar_reader_configuration_free(this->cReaderConfig); }

pulsar_reader_configuration_t *ReaderConfig::GetCReaderConfig() { return this->cReaderConfig; }

std::string ReaderConfig::GetTopic() { return this->topic; }

pulsar_message_id_t *ReaderConfig::GetCStartMessageId() { return this->cStartMessageId; }
