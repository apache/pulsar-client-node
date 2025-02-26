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
import sys
import requests
import os
import shutil
import zipfile
import tempfile
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
headers = {'Accept': ACCEPT_HEADER, 'Authorization': f'Bearer {GITHUB_TOKEN}'}

response = requests.get(workflow_run_url, headers=headers)
response.raise_for_status()

data = response.json()
for artifact in data['artifacts']:
    name = artifact['name']
    url = artifact['archive_download_url']

    print(f'Downloading {name} from {url}')
    artifact_response = requests.get(url, headers=headers, stream=True)
    artifact_response.raise_for_status()

    with tempfile.NamedTemporaryFile(delete=False) as tmp_zip:
        for chunk in artifact_response.iter_content(chunk_size=8192):
            tmp_zip.write(chunk)
        tmp_zip_path = tmp_zip.name

    try:
        dest_dir = os.path.join(dest_path, name)
        Path(dest_dir).mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(tmp_zip_path, 'r') as z:
            z.extractall(dest_dir)
    finally:
        os.unlink(tmp_zip_path)

for root, dirs, files in os.walk(dest_path, topdown=False):
    for name in files:
        shutil.move(os.path.join(root, name), dest_path)
    if not os.listdir(root):
        os.rmdir(root)