# AudioEngine
A simple but growing Linux audio engine based on Alsa

This project is a work in progress and is intended to give anyone the chance to have a quick look 
at a very basic audio engine structure, for teaching and creative purposes.

Some instruments and reserach projects I worked on were born out this simple but flexible piece of software. 
Other are still based on it and on its new experimental features, for example my latest instrument, the Hyper Drumhead.

Many many features come from other awesome open source projects and tutorials!
All these talented people are acknowledged and thanked in the related files.
Hope I did not forget anyone! If yes, please contact me and I will fix it on the spot.

Detailed documentation as well as new features are being added little by little.

If you have questions/comments, feel free to drop me a line:
victor.zappi@gmail.com

Victor_


//---------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------

HOW TO BUILD & RUN

_Dependencies:
asound
sndfile
fftw

e.g., on Ubuntu:
apt-get install libasound2-dev
apt-get install libsndfile1-dev
apt-get install libfftw3-dev


_Now you can build using the provided make file:
make


_Launch the newly built executable simply called AudioEngine
./AudioEngine
this plays a simple sine through the default audio card


Feel free to have a look at the source and play with it, starting from main.cpp 


Have fun (;

