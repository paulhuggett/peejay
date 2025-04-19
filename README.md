= peejay
JSON Parser for C++
:toc:

== Status

[cols="2,1"]
|===
| Continuous Integration 
| image:https://github.com/paulhuggett/peejay/actions/workflows/ci.yaml/badge.svg[CI Build & Test,link=https://github.com/paulhuggett/peejay/actions/workflows/ci.yaml]

| Static Analysis
| image:https://sonarcloud.io/api/project_badges/measure?project=paulhuggett_peejay&metric=alert_status[Quality Gate, link=https://sonarcloud.io/summary/new_code?id=paulhuggett_peejay] image:https://app.codacy.com/project/badge/Grade/a37157bbd85c440daadd8039cda137b2[Codacy Badge, link=https://app.codacy.com/gh/paulhuggett/peejay/dashboard]
image:https://img.shields.io/coverity/scan/28476.svg[Coverity Scan Build Status,link=https://scan.coverity.com/projects/paulhuggett-peejay]
image:https://github.com/paulhuggett/peejay/actions/workflows/msvc.yaml/badge.svg[Microsoft C++ Code Analysis,link=https://github.com/paulhuggett/peejay/actions/workflows/msvc.yaml]

| Dynamic Analysis
| image:https://github.com/paulhuggett/peejay/actions/workflows/klee.yaml/badge.svg[KLEE Tests,link=https://github.com/paulhuggett/peejay/actions/workflows/klee.yaml] image:https://codecov.io/github/paulhuggett/peejay/graph/badge.svg?token=BSNN6OFIJU[CodeCov,link=https://codecov.io/github/paulhuggett/peejay] image:https://github.com/paulhuggett/peejay/actions/workflows/fuzztest.yaml/badge.svg[Fuzz Tests,link=https://github.com/paulhuggett/peejay/actions/workflows/fuzztest.yaml]

| https://openssf.org[Open SSF]
| image:https://www.bestpractices.dev/projects/8006/badge[OpenSSF Best Practices,link=https://www.bestpractices.dev/projects/8006]
image:https://api.securityscorecards.dev/projects/github.com/paulhuggett/peejay/badge[OpenSSF Scorecard,link=https://securityscorecards.dev/viewer/?uri=github.com/paulhuggett/peejay]
|=== 

== Introduction

Peejay (PJ) is a state-machine based JSON parser for C++17 or later. (The silly name comes from the English pronunciation of P.J. which is short for **P**arse **J**SON.)
