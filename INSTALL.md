# Compile and Install shpc

## Prerequisites

- On macOS and Linux: A c++ compiler (clang)
- The libvxf

## Compiling on macOS

### Compile and Install
The folliwing procedure was last tested on an Apple M1 Pro (macOS 13.0.1), but it should also work on older systems.

Go to the project root and type the folling to compile the software:
~~~
make
~~~~

To install the software on your system, run:
~~~
sudo make install
~~~

The software will be located in "usr/local/bin".

To cleanup the build folders, run:
~~~
make clean
~~~

### Uninstall

To uninstall the software from the system, run:
~~~
sudo make remove
~~~
