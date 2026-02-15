# Cerverus

A high performance webserver made for linux first, should eventually also be able to be used for other types of servers such as game servers. Should also include an test-suite for testing servers, so maybe even the ability to DDoS (dunno if that will flag this project). The server will be based around htmx, with it's own templating engine.

## TODO

- allow for just making the assets/public public, also update the actual website
- Graceful failing for the most part once the server has been started.
- htmx callbacks
- create an client program that can be used for benchmarking
- Probably switch to a trie
- command buffer for the server-router interaction?
  - Important to allow for sendfile and other things...

## Prerequisites

- make
- clang
- Platform (Linux only, so far)

## Features

A prerelease version is working!

- Can host a static website
- Only allows for GET requests

### Upcoming

- platform wrapper
- filesystem
- updated filesystem and request handling with io_uring on linux to up performance

## Structure

First create a router structure. Then initialize a server, passing the router along. Add whatever routes you want to the router. Lastly start the server.

### Flow

Server: command buffer, router

Server receives request to connect.
Server checks firewall for IP and connection stuff.
Server passes request on to router.
Router checks in public directory if its a file request.
  if it is, send 404
Router then checks dynamic routes.
The route fills out the response struct, and the command buffer.
The router executes the command buffer.
  Command send HTTP response
  Send body if applicable
  Close connection if applicable

### Router

The router handles the way requests are processed and responses are sent. You will add routes to it.

### Server

The server parses requests into structures and passes them on to the router you passed it. This is also where you define stuff like protocols, ports, etc.

### Request

An request is an internal structure that can be parsed.

### Response

An response is an internal structure that can be serialized.

## References

Shoutout to the following people who's work has allowed for this project to be possible:

- Travis Vroman
- Nir Lichtman
- Jacob Sorber
