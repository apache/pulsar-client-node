<!--

    Licensed to the Apache Software Foundation (ASF) under one
    or more contributor license agreements.  See the NOTICE file
    distributed with this work for additional information
    regarding copyright ownership.  The ASF licenses this file
    to you under the Apache License, Version 2.0 (the
    "License"); you may not use this file except in compliance
    with the License.  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing,
    software distributed under the License is distributed on an
    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
    KIND, either express or implied.  See the License for the
    specific language governing permissions and limitations
    under the License.

-->
This page contains instructions for Pulsar committers on how to perform a release.

## Making the release

The steps for releasing are as follows:
1. Check and Add TypeScript Types
2. Create the release branch
3. Update package version and tag
4. Sign and stage the artifacts
5. Move master branch to next version
6. Publish the release candidate to the npm
7. Run the vote
8. Promote the release
9. Add release notes
10. Announce the release
 
## Versioning
Bump up the version number as follows.

* Major version (e.g. 1.8.0 => 2.0.0)
  * Changes that break backward compatibility
* Minor version (e.g. 1.8.0 => 1.9.0)
  * Backward compatible new features
* Patch version (e.g. 1.8.0 => 1.8.1)
  * Backward compatible bug fixes
  * C++ Client upgrade (even though there are no new commits in the Nodejs client)

## Requirements
If you haven't already done it, [create and publish the GPG key](https://pulsar.apache.org/contribute/releasing/create-gpg-keys) to sign the release artifacts.

## Steps in detail

#### 1. Check and Add TypeScript Types

In order to use added features and options in TypeScript code, we need to add their type definitions.

Check if there are any Pull Requests with the `types-required` label or not.
If yes, add their type definitions (e.g. https://github.com/apache/pulsar-client-node/pull/157).
If no, we can then proceed to next step.

#### 2. Create the release branch

We are going to create a branch from `master` to `branch-1.X` where the tag will be generated
and where new fixes will be applied as part of the maintenance for the release.

The branch needs only to be created when creating minor releases, and not for patch releases.

Eg: When creating `v1.1.0` release, will be creating the branch `branch-1.1`,
but for `v1.1.1` we would keep using the old `branch-1.1`.

In these instructions, I'm referring to an fictitious release `1.X.0`.
Change the release version in the examples accordingly with the real version.

It is recommended to create a fresh clone of the repository to avoid any local files to interfere
in the process:

```sh
$ git clone git@github.com:apache/pulsar-client-node.git
$ cd pulsar-client-node
$ git checkout -b branch-1.X origin/master
```

#### 3. Update package version and tag

During the release process, we are going to initially create "candidate" tags:
```sh
# Bump to the release version (the suffix "-rc.0" is removed)
$ npm version patch --no-git-tag-version
$ git add .
$ git commit -m 'Release v1.x.0'

# Create a "candidate" tag
$ export GPG_TTY=$(tty)
$ git tag -u $USER@apache.org v1.X.0-rc.1 -m 'Release v1.X.0-rc.1'

# Push both the branch and the tag to GitHub repo
$ git push origin branch-1.X
$ git push origin v1.X.0-rc.1
```

#### 4. Sign and stage the artifacts
The src artifact need to be signed and uploaded to the dist SVN repository for staging. See [here](https://github.com/apache/pulsar/wiki/Create-GPG-keys-to-sign-release-artifacts) for how to setup gpg.

When the build is pushed to the Apache repo, a Github action job will run and build all the binary artifacts.

The build will be available at `https://github.com/apache/pulsar-client-node/actions`

Once the workflow is completed (ETA ~20 min), the artifacts will be available for download.

Record the id of the workflow as `WORKFLOW_ID`.

All the artifacts need to be signed and uploaded to the dist SVN repository for staging.

Before running the script, make sure that the `user@apache.org` code signing key is the default gpg signing key. One way to ensure this is to create/edit file `~/.gnupg/gpg.conf` and add a line

```sh
default-key <key fingerprint>
```

where` <key fingerprint>` should be replaced with the private key fingerprint for the `user@apache.org` key. The key fingerprint can be found in `gpg -K` output.

```sh
$ svn co https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-node pulsar-dist-dev
$ cd pulsar-dist-dev

# '-rc.1' needs to be incremented in case of multiple iterations in getting
#    to the final release)
$ svn mkdir pulsar-client-node-1.X.0-rc.1
$ cd pulsar-client-node-1.X.0-rc.1

# Generate token from here: https://github.com/settings/tokens
$ export GITHUB_TOKEN=${Your github token} 
$ $PULSAR_PATH/build-support/stage-release.sh . $WORKFLOW_ID

$ svn add *
$ svn ci -m 'Staging artifacts and signature for Pulsar Node.js client release 1.X.0-rc.1'
```

#### 5. Move master branch to next version

We need to move master version to next iteration `X + 1`.

```sh
$ git checkout master
$ npm version preminor --preid=rc
```

Since this needs to be merged in `master`, we need to follow the regular process
and create a Pull Request on GitHub.

#### 6. Publish the release candidate to the npm

If you don't have permission to publish `pulsar-client` to the npm registry, ask other committers to grant that permission.

```sh
# Set the npm version for the release candidate
npm version 1.x.0-rc.1 --no-git-tag-version

# Create or verify a user account in the npm registry
$ npm adduser

Username: foobar
Password: ********
Email: (this IS public) foobar@apache.org

# Publish the rc version package to the npm registry
# We use the `rc` tag to distinguish the release candidate from the official release.
$ npm publish --tag rc
```

#### 7. Run the vote

Send an email on the Pulsar Dev mailing list:

```
To: dev@pulsar.apache.org
Subject: [VOTE] Pulsar Node.js Client Release 1.X.0 Candidate 1

Hi everyone,

This is the first release candidate for Apache Pulsar Node.js client, version 1.X.0.

It fixes the following issues:
https://github.com/apache/pulsar-client-node/milestone/1?closed=1

Please download the source files and review this release candidate:
- Download the source package, verify shasum and asc
- Follow the README.md to build and run the Pulsar Node.js client.

The release candidate package has been published to the npm registry:
https://www.npmjs.com/package/pulsar-client/v/1.X.0-rc.1
You can install it by `npm i pulsar-client@1.X.0-rc.1 --pulsar_binary_host_mirror=https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-node/` and verify the package.

The vote will be open for at least 72 hours. It is adopted by majority approval, with at least 3 PMC affirmative votes.

Source files:
https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-node/pulsar-client-node-1.X.0-rc.1/

Pulsar's KEYS file containing PGP keys we use to sign the release:
https://downloads.apache.org/pulsar/KEYS

SHA-512 checksum:
5f6c7e1a096a3ae66eee71c552af89532ed86bf94da3f3d49836c080426ee5dcaabeda440a989d51772d2e67e2dca953eeee9ea83cfbc7c2a0847a0ec04b310f  apache-pulsar-client-node-1.X.0.tar.gz

The tag to be voted upon:
v1.X.0-rc.1
https://github.com/apache/pulsar-client-node/releases/tag/v1.X.0-rc.1

Please review and vote on the release candidate #1 for the version 1.X.0, as follows:
[ ] +1, Approve the release
[ ] -1, Do not approve the release (please provide specific comments)
```

The vote should be open for at least 72 hours (3 days). Votes from Pulsar PMC members
will be considered binding, while anyone else is encouraged to verify the release and
vote as well.

If the release is approved here, we can then proceed to next step.

#### 8. Promote the release

Create the final git tag:
```sh
$ git checkout branch-1.X
$ export GPG_TTY=$(tty)
$ git tag -u $USER@apache.org v1.X.0 -m 'Release v1.X.0'
$ git push origin v1.X.0
```

Publish the release package:
```sh
# You can use `npm pack` and test the package before publishing.
# If there are no problems, then use `npm publish` to publish to the npm.
$ npm publish
```

Promote the artifacts on the release location (need PMC permissions):
```sh
$ svn mv -m 'Release Pulsar Node.js client 1.X.0' \
  https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-node/pulsar-client-node-1.X.0-rc.1 \
  https://dist.apache.org/repos/dist/release/pulsar/pulsar-client-node/pulsar-client-node-1.X.0

# Remove the old releases (if any)
# We only need the latest release there, older releases are available through the Apache archive
$ svn rm -m 'Remove the old release' \
  https://dist.apache.org/repos/dist/release/pulsar/pulsar-client-node/pulsar-client-node-1.Y.0
```

#### 9. Add release notes

Create a PR to add release notes to the pulsar site. Here is the example: https://github.com/apache/pulsar-site/pull/360
If there are any new contributors for this release, please add a `New Contributors` section in the release note.

Once the PR has been approved, add release notes to the Github release page (https://github.com/apache/pulsar-client-node/releases)

#### 10. Announce the release

Once the release artifact is available in the npm registry, we need to announce the release.

Send an email on these lines:
```
To: dev@pulsar.apache.org, users@pulsar.apache.org, announce@apache.org
Subject: [ANNOUNCE] Apache Pulsar Node.js client 1.X.0 released

The Apache Pulsar team is proud to announce Apache Pulsar Node.js client version 1.X.0.

Pulsar is a highly scalable, low latency messaging platform running on
commodity hardware. It provides simple pub-sub semantics over topics,
guaranteed at-least-once delivery of messages, automatic cursor management for
subscribers, and cross-datacenter replication.

For Pulsar Node.js client release details and downloads, visit:
https://www.npmjs.com/package/pulsar-client

Release Notes are at:
https://github.com/apache/pulsar-client-node/releases

We would like to thank the contributors that made the release possible.

Regards,

The Pulsar Team
```
