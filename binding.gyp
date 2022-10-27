#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

{
  "targets": [
    {
      "target_name": "Pulsar",
      "cflags_cc": ["-std=gnu++11"],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions", "-std=gnu++14", "-std=gnu++17"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
      ],
      "defines": ["NAPI_VERSION=4"],
      "sources": [
        "src/addon.cc",
        "src/Message.cc",
        "src/MessageId.cc",
        "src/Authentication.cc",
        "src/Client.cc",
        "src/SchemaInfo.cc",
        "src/Producer.cc",
        "src/ProducerConfig.cc",
        "src/Consumer.cc",
        "src/ConsumerConfig.cc",
        "src/Reader.cc",
        "src/ReaderConfig.cc",
        "src/ThreadSafeDeferred.cc"
      ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'GCC_ENABLE_CPP_RTTI': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '11.0',
            'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++11',
            'OTHER_CFLAGS': [
                "-fPIC",
            ]
          },
          "dependencies": [
            "<!@(node -p \"require('node-addon-api').gyp\")"
          ],
          "include_dirs": [
            "pkg/mac/build-pulsar/install/include"
          ],
        }],
        ['OS=="win"', {
          "defines": [
            "_HAS_EXCEPTIONS=1",
            "PULSAR_STATIC"
          ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1
            },
          },
          "include_dirs": [
            "pkg\\windows\\pulsar-cpp\\include",
          ],
          "libraries": [
            "..\\pkg\\windows\\pulsar-cpp\\lib\\pulsarWithDeps.lib"
          ],
          "dependencies": [
            "<!(node -p \"require('node-addon-api').gyp\")"
          ]
        }, {  # 'OS!="win"'
          "dependencies": [
            "<!@(node -p \"require('node-addon-api').gyp\")"
          ],
          "libraries": [
             "../pkg/lib/libpulsarwithdeps.a"
          ],
        }]
      ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
          "destination": "<(module_path)"
        }
      ]
    }
  ]
}
