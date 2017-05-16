# 
# Copyright (c) 2013-2017 Ardexa Pty Ltd
#
# This code is licensed under the MIT License (MIT).
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# CMAKE file for the Ardexa plugin to read an SMA Bluetooth enabled inverter
#
# Notes:
# 1.	Makesure the bluetooth dev library is installed.... via: sudo apt-get install -y libbluetooth-dev
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# Set the project name
PROJECT( ardexa-sma-bt)

# Set the output folder where your program will be created
set(CMAKE_BINARY_DIR .)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# Source files
set(ARDEXA_SMA_BT_SRC   src/main.cpp src/arguments.cpp src/utils.cpp)
set_source_files_properties(${ARDEXA_SMA_BT_SRC} PROPERTIES LANGUAGE CXX )

if(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x -fmessage-length=0")
	set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -fmessage-length=0 ")
endif()

# Build the application
add_executable(ardexa-sma-bt       ${ARDEXA_SMA_BT_SRC})
TARGET_LINK_LIBRARIES(ardexa-sma-bt m bluetooth)
SET_TARGET_PROPERTIES(ardexa-sma-bt PROPERTIES LINKER_LANGUAGE CXX)

# add the install targets
install (TARGETS ardexa-sma-bt DESTINATION /usr/local/bin)



