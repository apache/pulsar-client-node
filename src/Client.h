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

#ifndef CLIENT_H
#define CLIENT_H

#include <napi.h>
#include <pulsar/c/client.h>

struct LogMessage {
  pulsar_logger_level_t level;
  std::string file;
  int line;
  std::string message;

  LogMessage(pulsar_logger_level_t level, std::string file, int line, std::string message)
      : level(level), file(file), line(line), message(message) {}
};

struct LogCallback {
  Napi::ThreadSafeFunction callback;
};

void LogMessage(pulsar_logger_level_t level, const char *file, int line, const char *message, void *ctx);

class Client : public Napi::ObjectWrap<Client> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  Client(const Napi::CallbackInfo &info);
  ~Client();

 private:
  static Napi::FunctionReference constructor;
  pulsar_client_t *cClient;
  LogCallback *logCallback;

  Napi::Value CreateProducer(const Napi::CallbackInfo &info);
  Napi::Value Subscribe(const Napi::CallbackInfo &info);
  Napi::Value CreateReader(const Napi::CallbackInfo &info);
  Napi::Value Close(const Napi::CallbackInfo &info);
};

#endif
