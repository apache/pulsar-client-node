This page contains instructions for Pulsar committers on how to perform a release.

## Making the release

The steps for releasing are as follows:
1. Check and Add TypeScript Types
2. Create the release branch
3. Update package version and tag
4. Build and inspect the artifacts
5. Move master branch to next version
6. Sign and stage the artifacts
7. Write release notes
8. Run the vote
9. Promote the release
10. Update release notes
11. Announce the release

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

# Archive the source files
$ git archive HEAD --prefix=pulsar-client-node-1.X.0/ --output pulsar-client-node-1.X.0.tar.gz
```

#### 4. Build and inspect the artifacts

If you haven't done it, install the Pulsar C++ client according to the following document:
https://pulsar.apache.org/docs/en/client-libraries-cpp/

Install dependent npm modules and build Pulsar Node.js client:
```sh
$ npm ci
```

After the build, inspect the artifacts:

* Run the following command to verify the license headers in the source files:
```sh
$ npm run license:addheader

# If there is a file that does not contain the license header,
# the header is inserted by the above command
$ git diff
```
* Run the standalone Pulsar service and check that the Node.js client can connect to it correctly:
```sh
# Download the Pulsar binary distribution to any directory
$ cd /path/to/anydir
$ PULSAR_BIN_VER='x.y.z'
$ curl -L -o apache-pulsar-${PULSAR_BIN_VER}-bin.tar.gz "https://www.apache.org/dyn/mirrors/mirrors.cgi?action=download&filename=pulsar/pulsar-${PULSAR_BIN_VER}/apache-pulsar-${PULSAR_BIN_VER}-bin.tar.gz"

# Run the standalone service
$ tar xvzf apache-pulsar-${PULSAR_BIN_VER}-bin.tar.gz
$ cd apache-pulsar-${PULSAR_BIN_VER}
$ bin/pulsar standalone

# Open a new terminal and subscribe my-topic
$ cd /path/to/pulsar-client-node
$ node consumer.js

###### consumer.js example ######
const Pulsar = require('./index.js');

(async () => {
  const client = new Pulsar.Client({
    serviceUrl: 'pulsar://localhost:6650',
  });

  const consumer = await client.subscribe({
    topic: 'persistent://public/default/my-topic',
    subscription: 'sub1',
  });

  for (let i = 0; i < 10; i += 1) {
    const msg = await consumer.receive();
    console.log(msg.getData().toString());
    consumer.acknowledge(msg);
  }

  await consumer.close();
  await client.close();
})();
#################################

# Open another new terminal to produce messages into my-topic
$ cd /path/to/pulsar-client-node
$ node producer.js

###### producer.js example ######
const Pulsar = require('./index.js');

(async () => {
  const client = new Pulsar.Client({
    serviceUrl: 'pulsar://localhost:6650',
  });

  const producer = await client.createProducer({
    topic: 'persistent://public/default/my-topic',
  });

  for (let i = 0; i < 10; i += 1) {
    const msg = `my-message-${i}`;
    producer.send({
      data: Buffer.from(msg),
    });
    console.log(`Sent message: ${msg}`);
  }
  await producer.flush();

  await producer.close();
  await client.close();
})();
#################################
```

#### 5. Move master branch to next version

We need to move master version to next iteration `X + 1`.

```sh
$ git checkout master
$ npm version preminor --preid=rc
```

Since this needs to be merged in `master`, we need to follow the regular process
and create a Pull Request on GitHub.

#### 6. Sign and stage the artifacts

The src artifact need to be signed and uploaded to the dist SVN repository for staging. See [here](https://github.com/apache/pulsar/wiki/Create-GPG-keys-to-sign-release-artifacts) for how to setup gpg.

```sh
$ cd /path/to/anydir

$ svn co https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-node pulsar-dist-dev
$ cd pulsar-dist-dev

$ svn mkdir pulsar-client-node-1.X.0-candidate-1
$ cd pulsar-client-node-1.X.0-candidate-1

$ mv /path/to/pulsar-client-node/pulsar-client-node-1.X.0.tar.gz .

# Sign. (USER-ID is ID of the key to sign)
$ FILE=pulsar-client-node-1.X.0.tar.gz
$ gpg --armor --detach-sign --local-user USER-ID --output $FILE.asc $FILE
$ shasum -a 512 $FILE > $FILE.sha512

$ svn add *
$ svn ci -m 'Staging artifacts and signature for Pulsar Node.js client release 1.X.0-candidate-1'
```

#### 7. Write release notes

Check the milestone in GitHub associated with the release.
https://github.com/apache/pulsar-client-node/milestones?closed=1

In the release item, add the list of most important changes that happened in the release
and a link to the associated milestone, with the complete list of all the changes.
https://github.com/apache/pulsar-client-node/releases

#### 8. Run the vote

Send an email on the Pulsar Dev mailing list:

```
To: dev@pulsar.apache.org
Subject: [VOTE] Pulsar Node.js Client Release 1.X.0 Candidate 1

Hi everyone,
Please review and vote on the release candidate #1 for the version 1.X.0, as follows:
[ ] +1, Approve the release
[ ] -1, Do not approve the release (please provide specific comments)

This is the first release candidate for Apache Pulsar Node.js client, version 1.X.0.

It fixes the following issues:
https://github.com/apache/pulsar-client-node/milestone/1?closed=1

Please download the source files and review this release candidate:
- Review release notes
- Download the source package (verify shasum and asc) and follow the README.md to build and run the Pulsar Node.js client.

The vote will be open for at least 72 hours. It is adopted by majority approval, with at least 3 PMC affirmative votes.

Source files:
https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-node/pulsar-client-node-1.X.0-candidate-1/

Pulsar's KEYS file containing PGP keys we use to sign the release:
https://dist.apache.org/repos/dist/dev/pulsar/KEYS

SHA-512 checksum:
5f6c7e1a096a3ae66eee71c552af89532ed86bf94da3f3d49836c080426ee5dcaabeda440a989d51772d2e67e2dca953eeee9ea83cfbc7c2a0847a0ec04b310f  pulsar-client-node-1.X.0.tar.gz

The tag to be voted upon:
v1.X.0-rc.1
https://github.com/apache/pulsar-client-node/releases/tag/v1.X.0-rc.1
```

The vote should be open for at least 72 hours (3 days). Votes from Pulsar PMC members
will be considered binding, while anyone else is encouraged to verify the release and
vote as well.

If the release is approved here, we can then proceed to next step.

#### 9. Promote the release

Create the final git tag:
```sh
$ git checkout branch-1.X
$ export GPG_TTY=$(tty)
$ git tag -u $USER@apache.org v1.X.0 -m 'Release v1.X.0'
$ git push origin v1.X.0
```

Publish the release package:
```sh
# Create or verify a user account in the npm registry
$ npm adduser

Username: foobar
Password: ********
Email: (this IS public) foobar@apache.org

# Publish the package to the npm registry
$ npm publish

# If you don't have permission to publish `pulsar-client` to the npm registry,
# ask other committers to grant that permission.
```

Promote the artifacts on the release location (need PMC permissions):
```sh
$ svn mv -m 'Release Pulsar Node.js client 1.X.0' \
  https://dist.apache.org/repos/dist/dev/pulsar/pulsar-client-node/pulsar-client-node-1.X.0-candidate-1 \
  https://dist.apache.org/repos/dist/release/pulsar/pulsar-client-node/pulsar-client-node-1.X.0

# Remove the old releases (if any)
# We only need the latest release there, older releases are available through the Apache archive
$ svn rm -m 'Remove the old release' \
  https://dist.apache.org/repos/dist/release/pulsar/pulsar-client-node/pulsar-client-node-1.Y.0
```

#### 10. Update release notes

Add the release notes there:
https://github.com/apache/pulsar-client-node/releases

#### 11. Announce the release

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