cmake_minimum_required(VERSION 4.1)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
  set(CMAKE_CXX_STANDARD 23)
endif()

project ("Constellation" VERSION 0.1.1 DESCRIPTION "")

add_executable (Constellation "Constellation.cpp"  "UI.cpp"  "UI.hpp"  "rocket_model.hpp"  "SerialHandling.hpp"  "SerialHandling.cpp")

set_property(TARGET Constellation PROPERTY CXX_STANDARD 20)

set(LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
message( ${LIB_DIR} )

# Glad
set(GLAD_DIR "${LIB_DIR}/glad")

add_library(glad STATIC "${GLAD_DIR}/src/glad.c")

target_include_directories(glad PRIVATE "${GLAD_DIR}/include")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLAD_DIR}/include")
target_link_libraries(${PROJECT_NAME} glad "${CMAKE_DL_LIBS}")

# ImGui
set(IMGUI_DIR "${LIB_DIR}/ImGui")

add_library(ImGui STATIC "${IMGUI_DIR}/imgui.cpp"
"${IMGUI_DIR}/imgui_demo.cpp"
"${IMGUI_DIR}/imgui_draw.cpp"
"${IMGUI_DIR}/imgui_tables.cpp"
"${IMGUI_DIR}/imgui_widgets.cpp"
"${IMGUI_DIR}/implot_internal.h"
"${IMGUI_DIR}/implot_items.cpp"
"${IMGUI_DIR}/implot.cpp"
"${IMGUI_DIR}/implot.h"
"${IMGUI_DIR}/implot3d.h"
"${IMGUI_DIR}/implot3d.cpp"
"${IMGUI_DIR}/implot3d_internal.h"
"${IMGUI_DIR}/implot3d_items.cpp"

"${IMGUI_DIR}/imgui-knobs.h"
"${IMGUI_DIR}/imgui-knobs.cpp"

"${IMGUI_DIR}/imgui_impl_glfw.cpp"
"${IMGUI_DIR}/imgui_impl_opengl3.cpp")

target_link_libraries(ImGui PRIVATE glfw)

target_include_directories(ImGui PRIVATE "${IMGUI_DIR}")
target_include_directories(${PROJECT_NAME} PRIVATE "${IMGUI_DIR}")
target_link_libraries(${PROJECT_NAME} ImGui "${CMAKE_DL_LIBS}")

# Glfw3
set(GLFW_DIR "${LIB_DIR}/glfw3")

set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

add_subdirectory("${GLFW_DIR}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG_MODE)
endif()

target_link_libraries(${PROJECT_NAME} glfw "${GLFW_LIBRARIES}")
target_include_directories(${PROJECT_NAME} PRIVATE "${GLFW_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE "GLFW_INCLUDE_NONE")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/Assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Assets"
)