This tool allows you to compare versions of a Protocol Buffer specification
(entire .proto files or specific message and enum type).
The purpose is to see clearly whether the newer version maintains backwards compatibility and what new features it adds.

## Building

Prerequisites:

- CMake
- make
- C++ compiler
- Protocol Buffer libraries

Steps:

    mkdir build
    cd build
    cmake ..
    make

## Usage

    protobuf-spec-comparator dir1 file1.proto dir2 file2.proto type-name [--binary]

The program takes 5 arguments:

- dir1: A directory containing .proto files
- file1.proto: The relative path of a .proto file in dir1
- dir2 and file2.proto: The same as above for another version to compare.
- type-name: The name of a message or enum defined in file1.proto and file2.proto

You can add the following options:

- `--binary`: Report compatibility of the binary serialization as opposed to the JSON serialization or similar. See below for details.

### Behavior

The definition of a message or enum `type-name` in file1.proto and file2.proto is compared as detailed in the following sections.

If type-name is just ".", then all the messages and enums in file1.proto and file2.proto are compared.
Added and removed messages and enums are reported.

### Message comparison

Message fields are matched by name (default) or by number (when using `--binary`).

Fields present in file1 and missing in file2 are reported as removed, and vice-versa for added fields.
Any changes in matching fields are reported.

If two matching fields both have an enum or message type, then those enums and message types are also compared.

### Enum comparison

Enum values are matched by name (a binary comparison mode will be added where values are matched by ID number).

Values present in file1 and missing in file2 are reported as removed, and vice-versa for added fields.
Any changes in maching enum values are reported.
