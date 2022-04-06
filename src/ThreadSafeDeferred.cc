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

/**
 * MIT License
 *
 * Copyright (c) 2020 Michael K
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 * Adapted from https://github.com/0815fox/node-napi-threadsafe-deferred/tree/master/src
 */

#include "ThreadSafeDeferred.h"

std::shared_ptr<ThreadSafeDeferred> ThreadSafeDeferred::New(const Napi::Env env) {
  auto deferred = std::make_shared<ThreadSafeDeferred>(env);
  deferred->self = deferred;
  return deferred;
}

ThreadSafeDeferred::ThreadSafeDeferred(const Napi::Env env)
    : Deferred(env),
      fate(EFate::UNRESOLVED),
      createValueCb(NULL),
      errorMsg(""),
      tsf{Napi::ThreadSafeFunction::New(env, Napi::Function::New(env, [](const Napi::CallbackInfo &info) {}),
                                        "ThreadSafeDeferred", 0, 1, [this](Napi::Env env) {
                                          // this access happens from another thread.
                                          // However, no synchronization is needed as
                                          // the other thread cannot modify this instance
                                          // anymore after calling Resolve or Reject.
                                          if (this->fate == EFate::RESOLVED) {
                                            if (this->createValueCb == NULL) {
                                              Napi::Promise::Deferred::Resolve(env.Undefined());
                                            } else {
                                              Napi::Promise::Deferred::Resolve(this->createValueCb(env));
                                            }
                                          } else {
                                            Napi::Promise::Deferred::Reject(
                                                Napi::Error::New(env, this->errorMsg).Value());
                                          }

                                          this->self.reset();
                                        })} {}

void ThreadSafeDeferred::Resolve() {
  if (this->fate != EFate::UNRESOLVED) throw "Cannot resolve a promise which is not unresolved anymore.";
  this->fate = EFate::RESOLVED;
  this->tsf.Release();
}

void ThreadSafeDeferred::Resolve(const createValueCb_t createValueCb) {
  if (this->fate != EFate::UNRESOLVED) throw "Cannot resolve a promise which is not unresolved anymore.";
  this->createValueCb = createValueCb;
  this->fate = EFate::RESOLVED;
  this->tsf.Release();
}

void ThreadSafeDeferred::Reject(const std::string &errorMsg) {
  if (this->fate != EFate::UNRESOLVED) throw "Cannot reject a promise which is not unresolved anymore.";
  this->errorMsg = errorMsg;
  this->fate = EFate::REJECTED;
  this->tsf.Release();
}
