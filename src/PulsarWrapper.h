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

#ifndef PULSAR_CLIENT_NODE_PULSAR_WRAPPER_H_
#define PULSAR_CLIENT_NODE_PULSAR_WRAPPER_H_

#include <pulsar/ClientConfiguration.h>
#include <string>

namespace pulsar {

// Wrapper class to access private setDescription method
// This allows Node.js client to set client description
class PulsarWrapper {
 public:
  static void setClientDescription(ClientConfiguration& conf, const std::string& description);
};

} // namespace pulsar

#endif // PULSAR_CLIENT_NODE_PULSAR_WRAPPER_H_
