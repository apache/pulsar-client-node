#!/usr/bin/env python3
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

import sys, json, urllib.request, os, shutil, zipfile, tempfile
from pathlib import Path

if len(sys.argv) != 3:
    print("Usage: ")
    print("     %s $WORKFLOW_RUN_ID $DEST_PATH" % sys.argv[0])
    sys.exit(-1)

if 'GITHUB_TOKEN' not in os.environ:
    print('You should have a GITHUB_TOKEN environment variable')
    sys.exit(-1)

GITHUB_TOKEN = os.environ['GITHUB_TOKEN']

ACCEPT_HEADER = 'application/vnd.github+json'
LIST_URL = 'https://api.github.com/repos/apache/pulsar-client-node/actions/runs/%d/artifacts'

workflow_run_id = int(sys.argv[1])
dest_path = sys.argv[2]

workflow_run_url = LIST_URL % workflow_run_id
request = urllib.request.Request(workflow_run_url,
                    headers={'Accept': ACCEPT_HEADER, 'Authorization': 'Bearer ' + GITHUB_TOKEN})
with urllib.request.urlopen(request) as response:
    data = json.loads(response.read().decode("utf-8"))
    for artifact in data['artifacts']:
        name = artifact['name']
        url = artifact['archive_download_url']

        print('Downloading %s from %s' % (name, url))
        artifact_request = urllib.request.Request(url,
                    headers={'Authorization': 'Bearer ' + GITHUB_TOKEN})
        with urllib.request.urlopen(artifact_request) as response:
            tmp_zip = tempfile.NamedTemporaryFile(delete=False)
            try:
                #
                shutil.copyfileobj(response, tmp_zip)
                tmp_zip.close()

                dest_dir = os.path.join(dest_path, name)
                Path(dest_dir).mkdir(parents=True, exist_ok=True)
                with zipfile.ZipFile(tmp_zip.name, 'r') as z:
                    z.extractall(dest_dir)
            finally:
                os.unlink(tmp_zip.name)

    for root, dirs, files in os.walk(dest_path, topdown=False):
        for name in files:
            shutil.move(os.path.join(root, name), dest_path)
        if not os.listdir(root):
            os.rmdir(root)



