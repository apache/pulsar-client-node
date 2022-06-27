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
 */

#ifndef __THREADSAFE_DEFERRED_HPP
#define __THREADSAFE_DEFERRED_HPP

#include <napi.h>
#include <functional>

#define THREADSAFE_DEFERRED_RESOLVER(result) [=](const Napi::Env env) { return result; }

typedef std::function<Napi::Value(const Napi::Env env)> createValueCb_t;

class ThreadSafeDeferred : public Napi::Promise::Deferred {
 private:
  enum class EFate
  {
    UNRESOLVED,
    RESOLVED,
    REJECTED
  };
  EFate fate;
  createValueCb_t createValueCb;
  std::string errorMsg;
  Napi::ThreadSafeFunction tsf;
  std::shared_ptr<ThreadSafeDeferred> self;

 public:
  ThreadSafeDeferred(const Napi::Env env);

  void Resolve();  // <- if only Resolve were virtual... But we can live without polymorphism here
  void Resolve(const createValueCb_t);
  inline void Reject() { this->Reject(""); }
  void Reject(
      const std::string &);  // <- if only Reject were virtual... But we can live without polymorphism here

  static std::shared_ptr<ThreadSafeDeferred> New(const Napi::Env env);
};

struct ExtDeferredContext {
  ExtDeferredContext(std::shared_ptr<ThreadSafeDeferred> deferred) : deferred(deferred){};
  std::shared_ptr<ThreadSafeDeferred> deferred;
};

#endif /* __THREADSAFE_DEFERRED_HPP */
