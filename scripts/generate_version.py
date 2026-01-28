#!/usr/bin/env python3
import json

# Read version from package.json
with open('package.json', 'r') as f:
    version = json.load(f)['version']

# Write version to src/version.h
with open('src/version.h', 'w') as f:
    f.write(f'#define PULSAR_CLIENT_NODE_VERSION "{version}"\n')

print(f"Generated version.h with version: {version}")
