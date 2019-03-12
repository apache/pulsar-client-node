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

const commander = require('commander');
const delay = require('delay');
const {
  performance,
} = require('perf_hooks');
const Pulsar = require('../index.js');

// Parse args
(() => {
  commander
    .option('-s, --subscription [subscription]', 'Subscription')
    .option('-u, --url [url]', 'Pulsar Service URL')
    .option('-t, --topic [topic]', 'Topic Name')
    .option('-m, --messages [messages]', 'Number of Messages', 1000)
    .option('-i, --iteration [iteration]', 'Iteration of Measure', 3, parseInt)
    .on('--help', () => {
      console.log('');
      console.log('Examples:');
      console.log('  $ node perf/perf_consumer.js --subscription sub1 --url pulsar://localhost:6650 --topic persistent://public/default/my-topic');
    })
    .parse(process.argv);

  if (typeof commander.subscription === 'undefined') {
    console.error('no subscription given!');
    process.exit(1);
  }
  if (typeof commander.url === 'undefined') {
    console.error('no URL given!');
    process.exit(1);
  }
  if (typeof commander.topic === 'undefined') {
    console.error('no topic name given!');
    process.exit(1);
  }

  console.log('----------------------');
  commander.options.forEach((option) => {
    const optionName = (option.long).replace('--', '');
    console.log(`${optionName}: ${commander[optionName]}`);
  });
  console.log('----------------------');
})();

(async () => {
  // Create a client
  const clientConfig = {
    serviceUrl: commander.url,
  };
  const client = new Pulsar.Client(clientConfig);

  // Create a consumer
  const consumerConfig = {
    topic: commander.topic,
    subscription: commander.subscription,
  };
  const consumer = await client.subscribe(consumerConfig);

  const numOfMessages = parseInt(commander.messages, 10);
  for (let i = 0; i < commander.iteration; i += 1) {
    // measure
    await delay(1000);
    const startTimeMilliSeconds = performance.now();
    const results = [];
    for (let mi = 0; mi < numOfMessages; mi += 1) {
      results.push(new Promise((resolve) => {
        consumer.receive().then((msg) => {
          consumer.acknowledge(msg);
          resolve(msg.getData().length);
        });
      }));
    }
    const messageSizes = await Promise.all(results);
    const endTimeMilliSeconds = performance.now();

    // result
    const durationSeconds = (endTimeMilliSeconds - startTimeMilliSeconds) / 1000.0;
    const rate = numOfMessages / durationSeconds;
    const totalMessageSize = messageSizes.reduce((a, b) => a + b);
    const throuhputMbit = (totalMessageSize / 1024 / 1024 * 8) / durationSeconds;
    console.log('Throughput received: %f  msg/s --- %f Mbit/s', rate.toFixed(3), throuhputMbit.toFixed(3));
  }
  await consumer.close();
  await client.close();
})();
