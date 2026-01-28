# How to Release pulsar-client-node

## Overview

This document explains how to release a new version of pulsar-client-node, including how to update the version number in `binding.gyp` for the client description feature.

## Quick Steps for Release

### 1. Update version in package.json

Update the version number in `package.json`:

```bash
npm version <new-version>  # e.g., npm version 1.17.0
```

This automatically updates:
- `package.json` version field
- `package-lock.json` (if using npm/yarn)

### 2. Update version in binding.gyp

The version number is **hard-coded** in `binding.gyp` for the client description feature. This ensures the version is fixed at build time and doesn't require dynamic reading from package.json during the build process.

**Location in binding.gyp**:
```json
{
  "targets": [
    {
      "target_name": "pulsar",
      "defines": [
        "NAPI_VERSION=4",
        "PULSAR_CLIENT_NODE_VERSION=\"1.17.0-rc.0\""
      ]
    }
  ]
}
```

**IMPORTANT**: When updating to a new version, you must manually update the version string in `binding.gyp` to match the new version in `package.json`.

### 3. Run build and tests

```bash
# Clean previous build artifacts
rm -rf lib/ build/ Release/ obj.target/

# Install dependencies and build
npm install

# Run tests
npm test
```

### 4. Publish to npm (if applicable)

```bash
npm publish
```

## Client Description Feature

The client description feature automatically sets the client description to `"node-client-v<version>"` where `<version>` is the version from package.json.

This makes the Node.js client identify itself in broker topic statistics, displaying as:
```
Pulsar-CPP-vX.Y.Z-node-client-vA.B.C
```

## Example: Releasing Version 1.17.0

### Step 1: Update package.json
```bash
npm version 1.17.0
```

### Step 2: Update binding.gyp
Edit `binding.gyp` and change the version string from the old version to the new version:

```diff
- "PULSAR_CLIENT_NODE_VERSION=\"1.16.0\""
+ "PULSAR_CLIENT_NODE_VERSION=\"1.17.0\""
```

### Step 3: Commit changes
```bash
git add package.json binding.gyp
git commit -m "Bump version to 1.17.0"
```

### Step 4: Tag the release
```bash
git tag -a v1.17.0 -m "Release 1.17.0"
```

### Step 5: Push to remote
```bash
git push origin master
git push origin v1.17.0
```

## Automated Version Update Script

To help avoid manually updating `binding.gyp`, you can use the following script:

### `scripts/update_version.sh`

```bash
#!/bin/bash

# Get current version from package.json
CURRENT_VERSION=$(grep '"version"' package.json | head -1 | cut -d: -f4 | cut -d: -f2 | cut -d: -f2 | cut -d: -f3 | cut -d: -f2 | cut -d: -f3 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -当前的版本号会从package.json中读取并设置到 binding.gyp中
# Example: If package.json has "version": "1.17.0", binding.gyp will be updated to "PULSAR_CLIENT_NODE_VERSION=\"1.17.0\""

# 2. 重新生成所有的绑定文件 (如果需要)
# Regenerate binding files if needed (optional, for debugging or ensuring clean build)
rm -rf lib/ build/ Release/ obj.target/

# 3. 提示用户提交
# Prompt user to commit the changes
echo "Done! Version updated in binding.gyp."
echo "Please commit the changes with:"
echo "  git add package.json binding.gyp"
echo "  git commit -m 'Bump version to $(grep '\"version\"' package.json | head -1 | cut -d: -f4 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: - following: " \
  | head -1 | cut -d: -f4 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2 | cut -d: -f2编码问题，这个 bash 脚本有问题。让我重写：</think><tool_call>write<arg_key>content</arg_key><arg_value># How to Release pulsar-client-node

## Overview

This document explains how to release a new version of pulsar-client-node, including how to update the version number in `binding.gyp` for the client description feature.

## Quick Steps for Release

### 1. Update version in package.json

Update the version number in `package.json`:

```bash
npm version <new-version>  # e.g., npm version 1.17.0
```

This automatically updates:
- `package.json` version field
- `package-lock.json` (if using npm/yarn)

### 2. Update version in binding.gyp

The version number is **hard-coded** in `binding.gyp` for the client description feature. This ensures the version is fixed at build time and doesn't require dynamic reading from package.json during the build process.

**Location in binding.gyp**:
```json
{
  "targets": [
    {
      "target_name": "pulsar",
      "defines": [
        "NAPI_VERSION=4",
        "PULSAR_CLIENT_NODE_VERSION=\"1.17.0-rc.0\""
      ]
    }
  ]
}
```

**IMPORTANT**: When updating to a new version, you must manually update the version string in `binding.gyp` to match the new version in `package.json`.

### 3. Run build and tests

```bash
# Clean previous build artifacts
rm -rf lib/ build/ Release/ obj.target/

# Install dependencies and build
npm install

# Run tests
npm test
```

### 4. Publish to npm (if applicable)

```bash
npm publish
```

## Client Description Feature

The client description feature automatically sets the client description to `"node-client-v<version>"` where `<version>` is the version from package.json.

This makes the Node.js client identify itself in broker topic statistics, displaying as:
```
Pulsar-CPP-vX.Y.Z-node-client-vA.B.C
```

## Example: Releasing Version 1.17.0

### Step 1: Update package.json
```bash
npm version 1.17.0
```

### Step 2: Update binding.gyp
Edit `binding.gyp` and change the version string from the old version to the new version:

```diff
- "PULSAR_CLIENT_NODE_VERSION=\"1.16.0\""
+ "PULSAR_CLIENT_NODE_VERSION=\"1.17.0\""
```

### Step 3: Commit changes
```bash
git add package.json binding.gyp
git commit -m "Bump version to 1.17.0"
```

### Step 4: Tag the release
```bash
git tag -a v1.17.0 -m "Release 1.17.0"
```

### Step 5: Push to remote
```bash
git push origin master
git push origin v1.17.0
```

## Automated Version Update Script

To help with updating the version in `binding.gyp`, you can use the following script:

### `scripts/update_version.sh`

```bash
#!/bin/bash

# Get current version from package.json
CURRENT_VERSION=$(grep '"version"' package.json | head -1 | cut -d: -f4 | cut -d: -f2 | cut -d: -f2)

echo "Current version in package.json: $CURRENT_VERSION"
echo "This version will be set in binding.gyp."
read -p "Press Enter to continue, or Ctrl+C to cancel..."

# Extract version from package.json without quotes
VERSION=$(grep '"version"' package.json | head -1 | sed 's/.*"version"[[:space:]]*:[[:space:]]*//' | sed 's/"[[:space:]]*$//')

# Update binding.gyp with the new version
sed -i.bak "s/PULSAR_CLIENT_NODE_VERSION=\\\"[0-9.\\.]*/PULSAR_CLIENT_NODE_VERSION=\\\"$VERSION\\\"/g" binding.gyp

echo "Updated binding.gyp with version: $VERSION"
echo "Please review the changes with: git diff binding.gyp"
echo "Then commit with: git add binding.gyp && git commit -m 'Bump version to $VERSION'"
