################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../source/get_input/Get_Species.cpp \
../source/get_input/Handle_Mechanism_Input.cpp \
../source/get_input/Read_Input_Data_v2.cpp \
../source/get_input/Read_Reaction_Matrix.cpp \
../source/get_input/Read_Thermodynamic_Data_New_Format.cpp 

OBJS += \
./source/get_input/Get_Species.o \
./source/get_input/Handle_Mechanism_Input.o \
./source/get_input/Read_Input_Data_v2.o \
./source/get_input/Read_Reaction_Matrix.o \
./source/get_input/Read_Thermodynamic_Data_New_Format.o 

CPP_DEPS += \
./source/get_input/Get_Species.d \
./source/get_input/Handle_Mechanism_Input.d \
./source/get_input/Read_Input_Data_v2.d \
./source/get_input/Read_Reaction_Matrix.d \
./source/get_input/Read_Thermodynamic_Data_New_Format.d 


# Each subdirectory must supply rules for building sources it contributes
source/get_input/%.o: ../source/get_input/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"X:\workspace\CKS branches\headers" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

