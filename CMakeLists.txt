# ---------------------
# cmake settings
# ---------------------
cmake_minimum_required(VERSION 3.29.2)
set(CMAKE_CXX_STANDARD 20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ---------------------
# project
# ---------------------
project(source-diffusion LANGUAGES CXX)
add_subdirectory(vendor)

if(DEFINED BUILD_VERSION)
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
add_library(source-diffusion SHARED ${DLL_SOURCES} ${SHARED_SOURCES}
                                    res/resources.rc)
target_compile_definitions(source-diffusion PRIVATE ADDS _AMD64_
                                                    WIN32_LEAN_AND_MEAN)
target_link_libraries(
  source-diffusion
  PRIVATE stable-diffusion
          Zydis
          safetyhook
          nlohmann_json
          imgui
          imgui-filebrowser
          imgui-notify)
target_include_directories(source-diffusion PRIVATE "src/dll/" "src/shared")

# ---------------------
# loader
# ---------------------
file(GLOB_RECURSE LOADER_SOURCES CONFIGURE_DEPENDS "src/loader/*.cpp"
     "src/loader/*.h")
add_executable(source-diffusion-loader ${LOADER_SOURCES} ${SHARED_SOURCES})
add_dependencies(source-diffusion-loader source-diffusion)
target_compile_definitions(source-diffusion-loader PRIVATE ADDS _AMD64_
                                                           WIN32_LEAN_AND_MEAN)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
             PROPERTY VS_STARTUP_PROJECT source-diffusion-loader)
set_target_properties(
  source-diffusion-loader
  PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
             $<TARGET_FILE_DIR:source-diffusion-loader> OUTPUT_NAME
                                                        "Source diffusion")
target_include_directories(source-diffusion-loader PRIVATE "src/loader/"
                                                           "src/shared")

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
