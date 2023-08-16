# Compile and Install shpc

## Prerequisites

- On macOS and Linux: A c++ compiler (clang) and make
- The [libvxf](https://github.com/VertexFusion/libvxf) 

Your have to put both repositories, this and libvxf side by side in a folder, because this project is dependent on libvxf. 
The folder hirarchy is like this:
~~~
root/
├── libvxf/
├── shpc/
~~~

As long as we hadn't found a better solution, we work in this style.


## Compiling on macOS and Linux

### Compile and Install
The folliwing procedure was last tested on an Apple M1 Pro (macOS 13.0.1) and openSUSE-Leap-15.5-1, but it should also work on older systems.

~~~
git clone https://github.com/VertexFusion/libvxf.git
git clone https://github.com/VertexFusion/shpc.git
cd shpc
~~~

Go to the project root of shpc and type the folling to compile the software:
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
