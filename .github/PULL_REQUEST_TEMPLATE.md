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
<!--
### Contribution Checklist
  
  - PR title format should be *[type][component] summary*. For details, see *[Guideline - Pulsar PR Naming Convention](https://docs.google.com/document/d/1d8Pw6ZbWk-_pCKdOmdvx9rnhPiyuxwq60_TrD68d7BA/edit#heading=h.trs9rsex3xom)*. 

  - Fill out the template below to describe the changes contributed by the pull request. That will give reviewers the context they need to do the review.
  
  - Each pull request should address only one issue, not mix up code from multiple issues.
  
  - Each commit in the pull request has a meaningful commit message

  - Once all items of the checklist are addressed, remove the above text and this checklist, leaving only the filled out template below.
-->

<!-- Either this PR fixes an issue, -->

Fixes #<xyz>

<!-- or this PR is one task of an issue -->

Master Issue: #<xyz>

### Motivation

<!-- Explain here the context, and why you're making that change. What is the problem you're trying to solve. -->

### Modifications

<!-- Describe the modifications you've done. -->

### Verifying this change

- [ ] Make sure that the change passes the CI checks.

*(Please pick either of the following options)*

This change is a trivial rework / code cleanup without any test coverage.

*(or)*

This change is already covered by existing tests, such as *(please describe tests)*.

*(or)*

This change added tests and can be verified as follows:

*(example:)*
  - *Added integration tests for end-to-end deployment with large payloads (10MB)*
  - *Extended integration test for recovery after broker failure*

### Documentation

<!-- DO NOT REMOVE THIS SECTION. CHECK THE PROPER BOX ONLY. -->

- [ ] `doc-required` 
(Your PR needs to update docs and you will update later)

- [ ] `doc-not-needed` 
(Please explain why)

- [ ] `doc` 
(Your PR contains doc changes)

- [ ] `doc-complete`
(Docs have been already added)
