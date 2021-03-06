set(SRC_FILES
    src/ONCat.cpp
	src/ONCatEntity.cpp
	src/OAuth.cpp)

set(INC_FILES
    inc/MantidCatalog/Exception.h
    inc/MantidCatalog/ONCat.h
    inc/MantidCatalog/ONCatEntity.h
    inc/MantidCatalog/OAuth.h)

set(TEST_FILES
    ONCatTest.h
	ONCatEntityTest.h
	OAuthTest.h)

if(COVERAGE)
  foreach(loop_var ${SRC_FILES} ${INC_FILES})
    set_property(GLOBAL APPEND
                 PROPERTY COVERAGE_SRCS
                          "${CMAKE_CURRENT_SOURCE_DIR}/${loop_var}")
  endforeach(loop_var)
endif()

if(UNITY_BUILD)
  include(UnityBuild)
  enable_unity_build(Catalog SRC_FILES SRC_UNITY_IGNORE_FILES 10)
endif(UNITY_BUILD)

# Add ssl dependency
include_directories(${OPENSSL_INCLUDE_DIR})
add_definitions(-DWITH_OPENSSL -DWITH_NONAMESPACES)

# Add the target for this directory
add_library(Catalog ${SRC_FILES} ${INC_FILES})
# Set the name of the generated library
set_target_properties(Catalog
                      PROPERTIES OUTPUT_NAME
                                 MantidCatalog
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_CATALOG)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(Catalog
                        PROPERTIES INSTALL_RPATH "@loader_path/../MacOS;@loader_path/../Frameworks")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(Catalog
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

# Add to the 'Framework' group in VS
set_property(TARGET Catalog PROPERTY FOLDER "MantidFramework")

target_include_directories(Catalog PUBLIC inc/)

target_link_libraries(Catalog
                      LINK_PRIVATE
                      ${MANTIDLIBS}
                      ${OPENSSL_LIBRARIES}
                      ${JSONCPP_LIBRARIES})

# Add the unit tests directory
add_subdirectory(test)

# Auto-generate exports header
target_include_directories(Catalog PUBLIC ${CMAKE_BINARY_DIR}/Framework/Catalog)
generate_mantid_export_header(Catalog TRUE)

# Installation settings
mtd_install_targets(TARGETS
                    Catalog
                    INSTALL_DIRS
                    ${LIB_DIR}
                    ${WORKBENCH_LIB_DIR})
