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
const hdr = require('hdr-histogram-js');
const {
  performance,
} = require('perf_hooks');

const Pulsar = require('../index.js');

// Parse args
(() => {
  commander
    .option('-s, --size [size]', 'Message Size', 1024, parseInt)
    .option('-u, --url [url]', 'Pulsar Service URL')
    .option('-t, --topic [topic]', 'Topic Name')
    .option('-i, --iteration [iteration]', 'Iteration of Measure', 3, parseInt)
    .option('-m, --messages [messages]', 'Number of Messages', 1000, parseInt)
    .on('--help', () => {
      console.log('');
      console.log('Examples:');
      console.log('  $ node perf/perf_producer.js --url pulsar://localhost:6650 --topic persistent://public/default/my-topic');
    })
    .parse(process.argv);

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

// Produce
(async () => {
  // Create a client
  const clientConfig = {
    serviceUrl: commander.url,
  };
  const client = new Pulsar.Client(clientConfig);

  // Create a producer
  const producerConfig = {
    topic: commander.topic,
  };
  const producer = await client.createProducer(producerConfig);

  const sizeOfMessage = commander.size;
  const message = Buffer.alloc(sizeOfMessage, 1);
  const numOfMessages = commander.messages;
  for (let i = 0; i < commander.iteration; i += 1) {
    const histogram = hdr.build({
      bitBucketSize: 64,
      highestTrackableValue: 120000 * 1000,
      numberOfSignificantValueDigits: 5,
    });

    // measure
    await delay(1000);
    const startMeasureTimeMilliSeconds = performance.now();
    for (let mi = 0; mi < numOfMessages; mi += 1) {
      const startSendTimeMilliSeconds = performance.now();
      producer.send({
        data: message,
      }).then(() => {
        // add latency
        histogram.recordValue((performance.now() - startSendTimeMilliSeconds));
      });
    }
    await producer.flush();
    const endMeasureTimeMilliSeconds = performance.now();

    // result
    const elapsedSeconds = (endMeasureTimeMilliSeconds - startMeasureTimeMilliSeconds) / 1000;
    const rate = numOfMessages / elapsedSeconds;
    const throuhputMbit = (sizeOfMessage * numOfMessages / 1024 / 1024 * 8) / elapsedSeconds;
    console.log('Throughput produced: %f  msg/s --- %f Mbit/s --- Latency: mean: %f ms - med: %f - 95pct: %f - 99pct: %f - 99.9pct: %f - 99.99pct: %f - Max: %f',
      rate.toFixed(3),
      throuhputMbit.toFixed(3),
      histogram.mean,
      histogram.getValueAtPercentile(50),
      histogram.getValueAtPercentile(95),
      histogram.getValueAtPercentile(99),
      histogram.getValueAtPercentile(99.9),
      histogram.getValueAtPercentile(99.99),
      histogram.maxValue);
  }

  await producer.close();
  await client.close();
})();
