# Almost all CMake files should start with this
# You should always specify a range with the newest
# and oldest tested versions of CMake. This will ensure
# you pick up the best policies.
cmake_minimum_required(VERSION 3.1...3.25)

# This is your project statement. You should always list languages;
# Listing the version is nice here since it sets lots of useful variables
project(
  Scraper
  VERSION 1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)

# If you set any CMAKE_ variables, that can go here.
# (But usually don't do this, except maybe for C++ standard)

# Find packages go here.

# You should usually split this into folders, but this is a simple example

# This is a "default" library, and will match the *** variable setting.
# Other common choices are STATIC, SHARED, and MODULE
# Including header files here helps IDEs but is not required.
# Output libname matches target name, with the usual extensions on your system
add_library(Utils Utils.cpp Utils.hpp)
add_library(HTMLParser HTMLParser.cpp HTMLParser.hpp)

# Find packages
find_package(CURL REQUIRED)
find_package(LibXml2 REQUIRED)

# Include directories
# include_directories(/opt/homebrew/include)
# include_directories(/opt/homebrew/Cellar/libxml2/2.10.3_1/include)
include_directories(${CURL_INCLUDE_DIR})
include_directories(${LIBXML2_INCLUDE_DIR})

# Link directories
# link_directories(/opt/homebrew/Cellar/libxml2/2.10.3_1/lib)

# Adding something we can run - Output name matches target name
add_executable(Main Main.cpp)

# Make sure you link your targets with this command. It can also link libraries and
# even flags, so linking a target that does not exist will not give a configure-time error.
target_link_libraries(Main PRIVATE Utils)
target_link_libraries(Main PRIVATE HTMLParser)
target_link_libraries(Main PRIVATE ${CURL_LIBRARIES})
target_link_libraries(Main PRIVATE ${LIBXML2_LIBRARIES})