cmake_minimum_required(VERSION 3.10)

project(llconv VERSION 0.1
                  DESCRIPTION "Translates a LogicLab3 pll file to LogicLab5 plclib"
                  LANGUAGES CXX)

include_directories(source)
#file(GLOB SOURCES "source/*.cpp")
add_executable(llconv source/llconv.cpp)

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /O2 /utf-8 /J)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Werror -O3 -fno-rtti -funsigned-char)
endif()

target_compile_features(llconv PUBLIC cxx_std_17)

set_target_properties(llconv PROPERTIES
    #CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    #CXX_RTTI OFF
    INTERPROCEDURAL_OPTIMIZATION ON
)
#set_property(TARGET llconv PROPERTY INTERPROCEDURAL_OPTIMIZATION True)


#install(TARGETS llconv DESTINATION /usr/lib)
