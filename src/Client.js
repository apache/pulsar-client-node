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
const fs = require('fs');
const tls = require('tls');
const os = require('os');
const PulsarBinding = require('./pulsar-binding');

const certsFilePath = `${__dirname}/cert.pem`;

class Client {
  constructor(params) {
    if (!params.tlsTrustCertsFilePath) {
      // eslint-disable-next-line no-param-reassign
      params.tlsTrustCertsFilePath = certsFilePath;
    }
    this.client = new PulsarBinding.Client(params);
  }

  createProducer(params) {
    return this.client.createProducer(params);
  }

  subscribe(params) {
    return this.client.subscribe(params);
  }

  createReader(params) {
    return this.client.createReader(params);
  }

  getPartitionsForTopic(params) {
    return this.client.getPartitionsForTopic(params);
  }

  close() {
    this.client.close();
  }

  static setLogHandler(params) {
    PulsarBinding.Client.setLogHandler(params);
  }

  static genCertFile() {
    fs.rmSync(certsFilePath, { force: true });
    const fd = fs.openSync(certsFilePath, 'a');
    try {
      tls.rootCertificates.forEach((cert) => {
        fs.appendFileSync(fd, cert + os.EOL);
      });
    } finally {
      fs.closeSync(fd);
    }
  }
}

module.exports = Client;
