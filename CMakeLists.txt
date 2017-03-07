
add_definitions(--std=c++11)
find_package(cmakego REQUIRED COMPONENTS gstreamer REQUIRED)

add_executable(flow flow.cpp)

target_link_libraries(flow p::gstreamer)