cmake_minimum_required(VERSION 3.21)
include(AddGitSubmodule.cmake)
project(
	TressFXSSE
	VERSION 0.1.0
	LANGUAGES CXX
)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(XSEPlugin)

find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

find_package(xbyak CONFIG REQUIRED)

target_include_directories(
	"${PROJECT_NAME}"
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_BINARY_DIR}/cmake
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${SIMPLEINI_INCLUDE_DIRS}
		magic_enum::magic_enum
		nlohmann_json::nlohmann_json
)

find_package(xbyak CONFIG REQUIRED)
find_package( glm REQUIRED )
find_package(directx-headers CONFIG REQUIRED)
find_package(directxtk CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_git_submodule(extern/DirectXMesh)
target_include_directories(${PROJECT_NAME} 
	PUBLIC
		"extern/DirectXMesh/Utilities"
)
target_link_libraries(${PROJECT_NAME}
	PRIVATE
		debug ${CMAKE_CURRENT_SOURCE_DIR}/extern/detours/x64/Debug/detours.lib
		optimized ${CMAKE_CURRENT_SOURCE_DIR}/extern/detours/x64/Release/detours.lib
		xbyak::xbyak
		glm::glm
		d3dcompiler.lib
		Microsoft::DirectX-Guids 
		Microsoft::DirectX-Headers
		Microsoft::DirectXTK
		Microsoft::DirectXTex
		imgui::imgui
		nlohmann_json::nlohmann_json
		#D:/Modding/dev/TressFXSSE/build/vcpkg_installed/x64-windows-static-md/lib/Effects11.lib
)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
