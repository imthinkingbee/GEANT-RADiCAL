#----------------------------------------------------------------------------
# RADiCAL ultra-compact sampling calorimeter - GEANT4 simulation
#
# Geometry and physics based on:
#   C. Perez-Lara et al., "Study of time resolution measurements and prospects
#   for energy resolution of an ultra-compact sampling calorimeter (RADiCAL)
#   module at EM shower maximum over the energy range 25 GeV - 150 GeV",
#   Nucl. Instrum. Methods Phys. Res. A 1068 (2024) 169737.
#
# Build (requires a GEANT4 11.x install built with Qt/OpenGL + GDML optional):
#   mkdir build && cd build
#   cmake -DGeant4_DIR=/path/to/geant4/lib/Geant4-11.x ..
#   make -j
#   ./radical                 # interactive (vis)
#   ./radical run.mac         # batch
#----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.16...3.27)
project(radical LANGUAGES CXX)

#----------------------------------------------------------------------------
# Find Geant4. Optical photon transport requires no special component, but we
# request the UI/Vis drivers so the interactive mode works.
#----------------------------------------------------------------------------
option(WITH_GEANT4_UIVIS "Build with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Sources / headers
#----------------------------------------------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/include)
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(radical radical.cc ${sources} ${headers})
target_link_libraries(radical ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copy macro files to the build directory
#----------------------------------------------------------------------------
set(RADICAL_MACROS
  macros/init_vis.mac
  macros/vis.mac
  macros/run.mac
  macros/scan_energy.mac
  macros/scan_resolution.mac
  macros/scan_energy_fast.mac
  macros/scan_timing.mac
  macros/scan_array3x3.mac
  macros/scan_hex.mac
)
foreach(_mac ${RADICAL_MACROS})
  configure_file(${PROJECT_SOURCE_DIR}/${_mac} ${PROJECT_BINARY_DIR}/ COPYONLY)
endforeach()

install(TARGETS radical DESTINATION bin)
