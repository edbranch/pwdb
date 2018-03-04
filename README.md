# pwdb - A simple console based login/password manager

A login/password manager with a basic console interface and local-only
encrypted storage. Uses GnuPG for encryption and verification, leaving all key
management and encrypted file disemination and synchronization to the
user.

## Building

This project builds with cmake. Ex:

    mkdir build && cd build && cmake /path/to/pwdb && make

### Dependencies

#### Runtime

* GnuPG Made Easy (gpgme) - Encryption and signing
* GNU readline - Command input with editing and history
* Protocol Buffers (protobuf) - Storage format specification
* boost program\_options - Command line interface

#### Build

* CMake - Build system generation
* gcc - C++ compilation

