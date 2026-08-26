# Cerverus

A high performance webserver made for linux first, should eventually also be able to be used for other types of servers such as game servers. Should also include an test-suite for testing servers. The server will be based around htmx, with it's own templating engine.

## TODO

- refactor darray to be header based
- finish io_uring state machine
- create the filemap hashmap
- Bring back server config
- use GnuTLS for the SSL handshake
- Uploading/Downloading Files

## NOTE

- Protocol / Server Metadata: Date, Server, Connection
- Security / Controls: Cache-Control, X-Content-Type-Options
- Entity / Content Details: Content-Type, Content-Length, Content-Encoding

## Prerequisites

- make
- clang
- Platform (Linux only, so far)

## Features

A prerelease version is(was) working!

- Can host a dynamic website
- Only allows for GET requests

### Upcoming

- platform wrapper
- filesystem
- updated filesystem and request handling with io_uring on linux to up performance

## Benchmark (currently removed)

For a report to be created all unit tests must have passed as well as the smoke test. Specific stats are for a peak load test with the arguments seen in the report. Note that key changes since the last report are included.

## Structure

Create a router, the router provides a function that parses requests into readable form for it. Create a server_config (passing the router here), use this to create a server. The server exposes it's server_interface to the router, letting it know what commands it can call (sending, closing, etc.). Then run the server.

### router

The router handles routing based on the parsing function it requests the data to be parsed into. Internally it stores any dynamic routes in an trie, and static through an hashmap of the public directory.

### server

The server handles network, and general IO. (A lot.)

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
