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

const protobuf = require('protobufjs');
const descriptor = require('protobufjs/ext/descriptor');

const normalizeTypeName = (typeName) => typeName.replace(/^\./, '');

const createSchemaInfoFromRoot = ({
  root,
  rootMessageTypeName,
  rootFileDescriptorName,
  schemaType = 'ProtobufNative',
  syntax = 'proto3',
  name = rootMessageTypeName,
  properties = {},
}) => {
  if (!root) {
    throw new Error('root is required');
  }
  if (!rootMessageTypeName) {
    throw new Error('rootMessageTypeName is required');
  }
  if (!rootFileDescriptorName) {
    throw new Error('rootFileDescriptorName is required');
  }

  const normalizedTypeName = normalizeTypeName(rootMessageTypeName);
  const rootMessageType = root.lookupType(normalizedTypeName);
  const packageName = normalizedTypeName.split('.').slice(0, -1).join('.');
  const namespace = packageName ? root.lookup(packageName) : root;

  // protobufjs reflection JSON does not retain the source file name. Set it
  // before exporting a FileDescriptorSet, mirroring descriptor->file()->name().
  namespace.filename = rootFileDescriptorName;
  root.resolveAll();

  const fileDescriptorSet = root.toDescriptor(syntax);
  const fileDescriptorSetBytes = descriptor.FileDescriptorSet.encode(fileDescriptorSet).finish();

  return {
    schemaType,
    name,
    schema: JSON.stringify({
      fileDescriptorSet: Buffer.from(fileDescriptorSetBytes).toString('base64'),
      rootMessageTypeName: normalizeTypeName(rootMessageType.fullName),
      rootFileDescriptorName,
    }),
    properties,
  };
};

const createRootFromJson = (rootJson) => protobuf.Root.fromJSON(rootJson);

module.exports = {
  createRootFromJson,
  createSchemaInfoFromRoot,
};
