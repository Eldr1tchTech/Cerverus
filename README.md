# Cerverus

A high performance webserver made for linux first, should eventually also be able to be used for other types of servers such as game servers. Should also include an test-suite for testing servers. The server will be based around htmx, with it's own templating engine.

## TODO

- io_uring
- Go through includes in server.c
- Bring back server config
- Clean-up containers
 - add options to hashmap
 - make them configurable (custom compare function pointers), etc.
- Uploading/Downloading Files

## Prerequisites

- make
- clang
- Platform (Linux only, so far)

## Features

A prerelease version is working!

- Can host a dynamic website
- Only allows for GET requests

### Upcoming

- platform wrapper
- filesystem
- updated filesystem and request handling with io_uring on linux to up performance

## Benchmark

For a report to be created all unit tests must have passed as well as the smoke test. Specifc stats are for a peak load test with the arguments seen in the report. Note that key changes since the last report are included.

## Structure

Create a server, set it up, run it (more in depth documentation coming, for now just check out the default/example project).

### Server

The server parses requests into structures and passes them on to the router you passed it. This is also where you define stuff like protocols, ports, etc.

### Request

An request is an internal structure representing an request, that can be parsed.

### Response

An response is an internal structure representing an response, that can be serialized.

## References

Shoutout to the following people who's work has allowed for this project to be possible:

- Travis Vroman
- Nir Lichtman
- Jacob Sorber
- Tsoding
