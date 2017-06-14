add_definitions(--std=c++11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake_modules)
find_package(GLIB2 REQUIRED)
add_library(p::glib2 INTERFACE IMPORTED)
set_property(TARGET p::glib2 PROPERTY INTERFACE_LINK_LIBRARIES ${GLIB2_LIBRARIES} gobject-2.0)
set_property(TARGET p::glib2 PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GLIB2_INCLUDE_DIRS}")

find_package(GStreamer REQUIRED COMPONENTS app pbutils video)
add_library(p::gstreamer INTERFACE IMPORTED)
set_property(TARGET p::gstreamer PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GSTREAMER_INCLUDE_DIRS}" ${GSTREAMER_APP_INCLUDE_DIRS})
set_property(TARGET p::gstreamer PROPERTY INTERFACE_LINK_LIBRARIES ${GSTREAMER_LIBRARIES} ${GSTREAMER_APP_LIBRARIES} p::glib2)


add_executable(flow main.cpp)

target_link_libraries(flow p::gstreamer)