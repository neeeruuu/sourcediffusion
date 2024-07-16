# ---------------------
# cmake settings
# ---------------------
cmake_minimum_required(VERSION 3.29.2)
set(CMAKE_CXX_STANDARD 20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ---------------------
# project
# ---------------------
project(source-sd LANGUAGES CXX)
add_subdirectory(vendor)

if (DEFINED BUILD_VERSION)
	add_compile_definitions(SOURCEDIFFUSION_VER="${BUILD_VERSION}")
else()
	add_compile_definitions(SOURCEDIFFUSION_VER="development build")
endif()

# ---------------------
# shared globals
# ---------------------
file(GLOB_RECURSE SHARED_SOURCES CONFIGURE_DEPENDS "src/shared/*.cpp"
     "src/shared/*.h")

# ---------------------
# dll
# ---------------------
file(GLOB_RECURSE DLL_SOURCES CONFIGURE_DEPENDS "src/dll/*.cpp" "src/dll/*.h")
add_library(source-sd SHARED ${DLL_SOURCES} ${SHARED_SOURCES})
target_compile_definitions(source-sd PRIVATE ADDS _AMD64_ WIN32_LEAN_AND_MEAN)
target_link_libraries(
  source-sd
  PRIVATE stable-diffusion
          Zydis
          safetyhook
          nlohmann_json
          imgui
          imgui-filebrowser)
target_include_directories(source-sd PRIVATE "src/dll/" "src/shared")

# ---------------------
# loader
# ---------------------
file(GLOB_RECURSE LOADER_SOURCES CONFIGURE_DEPENDS "src/loader/*.cpp"
     "src/loader/*.h")
add_executable(source-sd-loader ${LOADER_SOURCES} ${SHARED_SOURCES})
add_dependencies(source-sd-loader source-sd)
target_compile_definitions(source-sd-loader PRIVATE ADDS _AMD64_
                                                    WIN32_LEAN_AND_MEAN)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT
                                                            source-sd-loader)
set_target_properties(
  source-sd-loader
  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:source-sd-loader>
             OUTPUT_NAME "Source SD")
target_include_directories(source-sd-loader PRIVATE "src/loader/" "src/shared")

# ---------------------
# folder grouping
# ---------------------
function(group_files sources)
  foreach(FILE ${sources})
    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" GROUP "${PARENT_DIR}"
    )# remove full dir
    string(REGEX MATCH "[(A-z)|(a-z)|(0-9)]+/[(A-z)|(a-z)|(0-9)]+" GROUPNAME
                 ${GROUP}) # get the first two folders from dir
    string(LENGTH ${GROUPNAME} GROUPLEN) # get project name len
    string(SUBSTRING ${GROUP} ${GROUPLEN} -1 GROUP) # remove project name from
                                                    # dir
    source_group("${GROUP}" FILES "${FILE}") # group file
  endforeach()
endfunction()

group_files("${DLL_SOURCES};${LOADER_SOURCES};${SHARED_SOURCES}")