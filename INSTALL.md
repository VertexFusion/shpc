# Compile and Install shpc

## Prerequisites

- On macOS and Linux: A c++ compiler (clang) and make
- The [libcore](https://github.com/VertexFusion/libcore) 

Your have to put both repositories, this and libcore side by side in a folder, because this project is dependent on libcore. 
The folder hierarchy is like this:
~~~
root/
├── libcore/
├── shpc/
~~~

## Compiling on macOS and Linux

### Compile and Install
The following procedure was last tested on an Apple M1 Pro (macOS 13.0.1) and openSUSE-Leap-15.5-1, but it should also work on older systems.

~~~
git clone --recursive  https://github.com/VertexFusion/libcore.git
git clone https://github.com/VertexFusion/shpc.git
cd shpc
~~~

Go to the project root of shpc and type the following to compile the software:
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
