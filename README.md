# Web Server in C

Simple and lightweight web server implemented in C.

## Features
- Handles basic HTTP requests.
- Logging capabilities.
- Extendable for future features.

## Getting Started

### Prerequisites
- GCC Compiler
- Basic knowledge in C programming

### Compilation

To compile the server, navigate to the source directory and run :
``````
gcc server.c -o server
``````
### Usage

To start the server, specify the ports for the HTTP server and for logging respectively: ``./server [HTTP_PORT] [LOGS_PORT]``

## Directory Structure

- `www/`: Contains web files served by the server.
- `logs/`: Directory where request logs are saved.