# AudioEngine
A simple but effective Linux audio engine based on Alsa.

This project is a work in progress and is intended to give anyone the chance to have a quick look 
at a very basic audio engine structure, for teaching and creative purposes.

Some instruments and reserach projects I worked on were born out this simple but flexible piece of software. 
Others are still based on it and on its new experimental features, for example my latest instrument, the [Hyper Drumhead](https://youtu.be/_qLwJC4PYR0?si=NYK2Z60hDYQrIfum).

Many many features come from other awesome open source projects and tutorials!
All these talented people are acknowledged and thanked in the related files.
Hope I did not forget anyone! If yes, please contact me and I will fix it on the spot.

As usual, not much time for documentation, but the provided examples may help!
And yes, there are many warnings and bad habits lying around.

If you have questions/comments, feel free to drop me a line:
v.zappi@northeastern.edu

Victor_


//---------------------------------------------------------------------------------------------------------------


## How to Build and Run

**_Dependencies:**
- CMake

- libs:
    - asound
    - sndfile
    - fftw


e.g., on Ubuntu:
```console
apt install cmake libasound2-dev libsndfile1-dev libfftw3-dev
```

**_Now you can choose a project and build it like this:**
```console
mkdir build && cd build
cmake -DPRJ_DIR=path/to/project
make
```
Notice that:
- The path to the project must be relative to the *AudioEngine* directory, where the CMakeLists.txt file is located
- An AudioEngine project must have a *main.cpp* file and should include *"AudioEngine.h"*. Check the provided examples!
- The path should end with the directory that contains the main file, e.g., *examples/sine*
- If everything goes right, the *make* command will generate a binary in the newly created *AudioEngine/bin* directory
- The name of the binary will be the name of the last directory within the project's path


**_Launch the bin:**
```console
cd ../bin
./sine
```
in this case, we built and run the *sine* example project, which plays a simple sine through the default audio card, until ctrl-c is pressed!


Feel free to have a look at the source and play with it, starting from the examples.


Have fun (;

