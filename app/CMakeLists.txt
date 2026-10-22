cmake_minimum_required(VERSION 3.13.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C CXX)
target_sources(app PRIVATE
        src/main.cpp
        src/drv8844.cpp
)
target_include_directories(
        app
        PRIVATE
        include
        lib/AX12
        lib/BrushlessMotor
        lib/Buttons
        lib/CRC16
        lib/Elevator
        lib/EndStop
        lib/FullSpeedServo
        lib/Grafcet/include
        lib/HazelnutGripper/include
        lib/Include
        lib/Logger
        lib/mathutils
        lib/motors/include
        lib/Musique
        lib/PeriodicProcess
        lib/SensorArray
        lib/ServosPCA9685
        lib/ShiftRegister
        lib/SpeedPowerProfiling
        lib/StepByStepMotor
        lib/STM32HALTimer
        lib/Teleplot
        lib/Wheeledbase/include
)