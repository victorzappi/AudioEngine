# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
./main.cpp 

OBJS += \
build/main.o 

CPP_DEPS += \
build/main.d 


# Each subdirectory must supply rules for building sources it contributes
build/%.o: ./%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
#	g++ -I"/home/vic/Digital_Duets/Digital_Duets_workspace/_Core/include" -O0 -g3 -Wall -c -fmessage-length=0 -pthread  -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	g++ -I"./include" -O0 -g3 -Wall -c -fmessage-length=0 -pthread  -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


