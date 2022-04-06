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

#ifndef READER_CONFIG_H
#define READER_CONFIG_H

#include <napi.h>
#include <pulsar/c/reader.h>
#include <pulsar/c/reader_configuration.h>
#include <pulsar/c/message_id.h>
#include "ReaderListener.h"

class ReaderConfig {
 public:
  ReaderConfig(const Napi::Object &readerConfig, pulsar_reader_listener readerListener);
  ~ReaderConfig();
  std::shared_ptr<pulsar_reader_configuration_t> GetCReaderConfig();
  std::shared_ptr<pulsar_message_id_t> GetCStartMessageId();
  std::string GetTopic();
  ReaderListenerCallback *GetListenerCallback();

 private:
  std::string topic;
  std::shared_ptr<pulsar_message_id_t> cStartMessageId;
  std::shared_ptr<pulsar_reader_configuration_t> cReaderConfig;
  ReaderListenerCallback *listener;
};

#endif
