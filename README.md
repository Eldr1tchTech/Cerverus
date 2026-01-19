# Cerverus

A high performance webserver made for linux first, should eventually also be able to be used for other types of servers such as game servers. Should also include an test-suite for testing servers, so maybe even the ability to DDoS (dunno if that will flag this project). The server will be based around htmx, with it's own templating engine.

## TODO

- Finish request parsing and response serialization.
- Move appropriate code into the router/server
- Make sure that works
- Either switch to trie, or decide to make a command buffer for the server-router interaction.

## Prerequisites

- make
- clang
- Platform (Linux only, so far)

## Features

For now there are very few features, but a lot of upcoming features.

### Upcoming

- platform wrapper
- proper request handling
- proper routing
- filesystem
- updated filesystem and request handling with io_uring on linux to up performance

## Structure

First create a router structure. Then initialize a server, passing the router along. Add whatever routes you want to the router. Lastly start the server.

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
