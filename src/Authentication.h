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

#ifndef AUTH_H
#define AUTH_H

#include <napi.h>
#include <pulsar/c/authentication.h>

class Authentication : public Napi::ObjectWrap<Authentication> {
 public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  Authentication(const Napi::CallbackInfo &info);
  ~Authentication();
  pulsar_authentication_t *GetCAuthentication();

 private:
  static Napi::FunctionReference constructor;
  pulsar_authentication_t *cAuthentication;
};

#endif
