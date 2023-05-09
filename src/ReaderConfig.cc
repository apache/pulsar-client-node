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
#include <pulsar/c/consumer_configuration.h>
#include <map>

static const std::string CFG_TOPIC = "topic";
static const std::string CFG_START_MESSAGE_ID = "startMessageId";
static const std::string CFG_RECV_QUEUE = "receiverQueueSize";
static const std::string CFG_READER_NAME = "readerName";
static const std::string CFG_SUBSCRIPTION_ROLE_PREFIX = "subscriptionRolePrefix";
static const std::string CFG_READ_COMPACTED = "readCompacted";
static const std::string CFG_LISTENER = "listener";
static const std::string CFG_PRIVATE_KEY_PATH = "privateKeyPath";
static const std::string CFG_CRYPTO_FAILURE_ACTION = "cryptoFailureAction";

static const std::map<std::string, pulsar_consumer_crypto_failure_action> CONSUMER_CRYPTO_FAILURE_ACTION = {
    {"FAIL", pulsar_ConsumerFail},
    {"DISCARD", pulsar_ConsumerDiscard},
    {"CONSUME", pulsar_ConsumerConsume},
};

void FinalizeListenerCallback(Napi::Env env, ReaderListenerCallback *cb, void *) { delete cb; }

ReaderConfig::ReaderConfig(const Napi::Object &readerConfig, pulsar_reader_listener readerListener)
    : topic(""), cStartMessageId(NULL), listener(nullptr) {
  this->cReaderConfig = std::shared_ptr<pulsar_reader_configuration_t>(pulsar_reader_configuration_create(),
                                                                       pulsar_reader_configuration_free);

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
      pulsar_reader_configuration_set_receiver_queue_size(this->cReaderConfig.get(), receiverQueueSize);
    }
  }

  if (readerConfig.Has(CFG_READER_NAME) && readerConfig.Get(CFG_READER_NAME).IsString()) {
    std::string readerName = readerConfig.Get(CFG_READER_NAME).ToString().Utf8Value();
    if (!readerName.empty())
      pulsar_reader_configuration_set_reader_name(this->cReaderConfig.get(), readerName.c_str());
  }

  if (readerConfig.Has(CFG_SUBSCRIPTION_ROLE_PREFIX) &&
      readerConfig.Get(CFG_SUBSCRIPTION_ROLE_PREFIX).IsString()) {
    std::string subscriptionRolePrefix =
        readerConfig.Get(CFG_SUBSCRIPTION_ROLE_PREFIX).ToString().Utf8Value();
    if (!subscriptionRolePrefix.empty())
      pulsar_reader_configuration_set_reader_name(this->cReaderConfig.get(), subscriptionRolePrefix.c_str());
  }

  if (readerConfig.Has(CFG_READ_COMPACTED) && readerConfig.Get(CFG_READ_COMPACTED).IsBoolean()) {
    bool readCompacted = readerConfig.Get(CFG_READ_COMPACTED).ToBoolean();
    if (readCompacted) {
      pulsar_reader_configuration_set_read_compacted(this->cReaderConfig.get(), 1);
    }
  }

  if (readerConfig.Has(CFG_LISTENER) && readerConfig.Get(CFG_LISTENER).IsFunction()) {
    this->listener = new ReaderListenerCallback();
    Napi::ThreadSafeFunction callback = Napi::ThreadSafeFunction::New(
        readerConfig.Env(), readerConfig.Get(CFG_LISTENER).As<Napi::Function>(), "Reader Listener Callback",
        1, 1, (void *)NULL, FinalizeListenerCallback, listener);
    this->listener->callback = std::move(callback);
    pulsar_reader_configuration_set_reader_listener(this->cReaderConfig.get(), readerListener,
                                                    this->listener);
  }

  if (readerConfig.Has(CFG_PRIVATE_KEY_PATH) && readerConfig.Get(CFG_PRIVATE_KEY_PATH).IsString()) {
    std::string publicKeyPath = "";
    std::string privateKeyPath = readerConfig.Get(CFG_PRIVATE_KEY_PATH).ToString().Utf8Value();
    pulsar_reader_configuration_set_default_crypto_key_reader(this->cReaderConfig.get(),
                                                              publicKeyPath.c_str(), privateKeyPath.c_str());
    if (readerConfig.Has(CFG_CRYPTO_FAILURE_ACTION) &&
        readerConfig.Get(CFG_CRYPTO_FAILURE_ACTION).IsString()) {
      std::string cryptoFailureAction = readerConfig.Get(CFG_CRYPTO_FAILURE_ACTION).ToString().Utf8Value();
      if (CONSUMER_CRYPTO_FAILURE_ACTION.count(cryptoFailureAction)) {
        pulsar_reader_configuration_set_crypto_failure_action(
            this->cReaderConfig.get(), CONSUMER_CRYPTO_FAILURE_ACTION.at(cryptoFailureAction));
      }
    }
  }
}

ReaderConfig::~ReaderConfig() {
  if (this->listener != nullptr) {
    this->listener->callback.Release();
  }
}

std::shared_ptr<pulsar_reader_configuration_t> ReaderConfig::GetCReaderConfig() {
  return this->cReaderConfig;
}

std::string ReaderConfig::GetTopic() { return this->topic; }

std::shared_ptr<pulsar_message_id_t> ReaderConfig::GetCStartMessageId() { return this->cStartMessageId; }

ReaderListenerCallback *ReaderConfig::GetListenerCallback() {
  ReaderListenerCallback *cb = this->listener;
  this->listener = nullptr;
  return cb;
}
