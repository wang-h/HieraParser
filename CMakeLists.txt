# -*- CMakeLists.txt. Do not edit by hand -*-
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

cmake_minimum_required(VERSION 3.5.1)
project(HieraParser)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

# Compiler options
find_package(Threads)
add_definitions(-w)
add_definitions(-std=c++11) 
add_definitions(-pthread)
add_definitions(-Wall)
add_definitions(-O4)
add_definitions(-DNDEBUG) 
#
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -L. ")

add_subdirectory(src)


if(WIN32)
    enable_language(RC)
    set(CMAKE_RC_COMPILE_OBJECT
        "<CMAKE_RC_COMPILER> ${RC_OPTIONS} -O coff -i <SOURCE> -o <OBJECT>")
endif(WIN32)

# train
add_executable(train ${RC_SRCS} src/train.cc)
target_link_libraries(train HIERAP_MAIN ${RC_SRCS} ${LINK_OPTIONS}  ${CMAKE_THREAD_LIBS_INIT})

# parse
add_executable(parse ${RC_SRCS} src/parse.cc )
target_link_libraries(parse HIERAP_MAIN ${RC_SRCS} ${LINK_OPTIONS}  ${CMAKE_THREAD_LIBS_INIT})

