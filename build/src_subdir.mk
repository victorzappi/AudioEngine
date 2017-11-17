# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
./src/ADSR.cpp \
./src/AnalogInput.cpp \
./src/AudioEngine.cpp \
./src/AudioEngine_render.cpp \
./src/Biquad.cpp \
./src/FFT.cpp \
./src/I2c_TouchKey.cpp \
./src/Midi.cpp \
./src/OSCClient.cpp \
./src/OSCServer.cpp \
./src/Oscillator.cpp \
./src/PinkNoise.cpp \
./src/UdpClient.cpp \
./src/UdpServer.cpp \
./src/Waveforms.cpp \
./src/priority_utils.cpp \
./src/strnatcmp.cpp 

OBJS += \
build/src/ADSR.o \
build/src/AnalogInput.o \
build/src/AudioEngine.o \
build/src/AudioEngine_render.o \
build/src/Biquad.o \
build/src/FFT.o \
build/src/I2c_TouchKey.o \
build/src/Midi.o \
build/src/OSCClient.o \
build/src/OSCServer.o \
build/src/Oscillator.o \
build/src/PinkNoise.o \
build/src/UdpClient.o \
build/src/UdpServer.o \
build/src/Waveforms.o \
build/src/priority_utils.o \
build/src/strnatcmp.o 

CPP_DEPS += \
build/src/ADSR.d \
build/src/AnalogInput.d \
build/src/AudioEngine.d \
build/src/AudioEngine_render.d \
build/src/Biquad.d \
build/src/FFT.d \
build/src/I2c_TouchKey.d \
build/src/Midi.d \
build/src/OSCClient.d \
build/src/OSCServer.d \
build/src/Oscillator.d \
build/src/PinkNoise.d \
build/src/UdpClient.d \
build/src/UdpServer.d \
build/src/Waveforms.d \
build/src/priority_utils.d \
build/src/strnatcmp.d 


# Each subdirectory must supply rules for building sources it contributes
build/src/%.o: ./src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
#	g++ -I"/home/vic/Digital_Duets/Digital_Duets_workspace/_Core/include" -O0 -g3 -Wall -c -fmessage-length=0 -pthread  -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	g++ -I"./include" -O0 -g3 -Wall -c -fmessage-length=0 -pthread  -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


