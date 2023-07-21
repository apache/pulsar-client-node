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

const PulsarBinding = require('./src/pulsar-binding');
const AuthenticationTls = require('./src/AuthenticationTls');
const AuthenticationAthenz = require('./src/AuthenticationAthenz');
const AuthenticationToken = require('./src/AuthenticationToken');
const AuthenticationOauth2 = require('./src/AuthenticationOauth2');
const Client = require('./src/Client');

const LogLevel = {
  DEBUG: 0,
  INFO: 1,
  WARN: 2,
  ERROR: 3,
  toString: (level) => Object.keys(LogLevel).find((key) => LogLevel[key] === level),
};

const Pulsar = {
  Client,
  Message: PulsarBinding.Message,
  MessageId: PulsarBinding.MessageId,
  AuthenticationTls,
  AuthenticationAthenz,
  AuthenticationToken,
  AuthenticationOauth2,
  LogLevel,
};

module.exports = Pulsar;
