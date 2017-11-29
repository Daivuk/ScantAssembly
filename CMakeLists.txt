cmake_minimum_required(VERSION 3.5)

project(ScantAssembly)

if (WIN32)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

add_executable(ScantAssembly WIN32
    src/main.cpp
	src/imgui.cpp
	src/imgui_demo.cpp
	src/imgui_draw.cpp
	src/TextEditor.h
	src/TextEditor.cpp
	src/levels.h
	src/constants.h
	src/dictionnary.h
    src/http.h
    src/help.h
)

if (WIN32)
    list(APPEND libs
        PUBLIC
            Mfplat
            msxml6
            comsuppwd
            Winhttp
    )
endif()

find_package(OpenGL)
if (OPENGL_FOUND)
    include_directories(
		./src
		${OPENGL_INCLUDE_DIR})
    list(APPEND libs ${OPENGL_LIBRARIES})
endif (OPENGL_FOUND)

target_link_libraries(ScantAssembly ${libs})
