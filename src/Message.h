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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <napi.h>
#include <pulsar/c/message.h>

class Message : public Napi::ObjectWrap<Message> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::Object NewInstance(Napi::Value arg, std::shared_ptr<pulsar_message_t> cMessage);
  static std::shared_ptr<pulsar_message_t> BuildMessage(Napi::Object conf);
  Message(const Napi::CallbackInfo &info);
  ~Message();
  std::shared_ptr<pulsar_message_t> GetCMessage();

 private:
  static Napi::FunctionReference constructor;

  std::shared_ptr<pulsar_message_t> cMessage;

  Napi::Value GetTopicName(const Napi::CallbackInfo &info);
  Napi::Value GetProperties(const Napi::CallbackInfo &info);
  Napi::Value GetData(const Napi::CallbackInfo &info);
  Napi::Value GetMessageId(const Napi::CallbackInfo &info);
  Napi::Value GetPublishTimestamp(const Napi::CallbackInfo &info);
  Napi::Value GetEventTimestamp(const Napi::CallbackInfo &info);
  Napi::Value GetPartitionKey(const Napi::CallbackInfo &info);
  Napi::Value GetRedeliveryCount(const Napi::CallbackInfo &info);
  bool ValidateCMessage(Napi::Env env);

  static char **NewStringArray(int size) { return (char **)calloc(sizeof(char *), size); }
  static void SetString(char **array, const char *str, int n) {
    char *copied = (char *)calloc(strlen(str) + 1, sizeof(char));
    strcpy(copied, str);
    array[n] = copied;
  }
  static void FreeStringArray(char **array, int size) {
    int i;
    for (i = 0; i < size; i++) {
      free(array[i]);
    }
    free(array);
  }
};

#endif
