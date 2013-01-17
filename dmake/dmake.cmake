# dmake is written by Dan Furse (dlfurse@mit.edu)

# the goal of dmake is to help you stay away from wanting to rip your balls off and bleed out when you use cmake
message( STATUS "*** dmake! take it easy:-D ***" )
message( STATUS "dmake will be used to build ${PROJECT_NAME}")

# deal with the dmake cache; check if there's a dmake cache on this computer and determine whether or not to use it
if( NOT DEFINED DMAKE_CACHE_DIR )
    set( DMAKE_CACHE_DIR "$ENV{HOME}/.dmake" CACHE PATH "Location of the dmake cache" )
endif( NOT DEFINED DMAKE_CACHE_DIR )
if( NOT DEFINED DMAKE_USE_CACHE )
    set( DMAKE_USE_CACHE FALSE CACHE BOOL "Flag to indicate if the dmake cache should be used" )
    set( DMAKE_CACHE_EXISTS FALSE )
endif( NOT DEFINED DMAKE_USE_CACHE )

if( EXISTS ${DMAKE_CACHE_DIR} )
    set( DMAKE_CACHE_EXISTS TRUE )
    set( DMAKE_USE_CACHE TRUE )
else( EXISTS ${DMAKE_CACHE_DIR} )
    set( DMAKE_CACHE_EXISTS FALSE )
endif( EXISTS ${DMAKE_CACHE_DIR} )

if( NOT ${DMAKE_CACHE_EXISTS} AND ${DMAKE_USE_CACHE} )
    message( STATUS "Creating dmake cache direcory: ${DMAKE_CACHE_DIR}" )
    file( MAKE_DIRECTORY ${DMAKE_CACHE_DIR} )
    set( DMAKE_CACHE_EXISTS TRUE )
    set( DMAKE_USE_CACHE TRUE )
endif( NOT ${DMAKE_CACHE_EXISTS} AND ${DMAKE_USE_CACHE} )

# check if this is a stand-alone build
set( DM_STANDALONE FALSE CACHE INTERNAL "Flag for whether or not this is a stand-alone build" )
if( ${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR} )
    set( DM_STANDALONE TRUE )
endif( ${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR} )

# indicate to this and any nested builds that this is a dmake-based build
if( ${DM_BUILD} )
    set( DM_PARENT_PROJECT_NAME ${DM_PROJECT_NAME} )
endif( ${DM_BUILD} )
set( DM_BUILD TRUE )

# this macro declares a project and declares some installation and structural cache variables so you can tell cmake what you want
macro( dmake_project_begin LOCAL_PROJECT_NAME LOCAL_MAJOR_VERSION LOCAL_MINOR_VERSION LOCAL_REVISION_VERSION LOCAL_HEADER_SUBDIRECTORY LOCAL_SOURCE_SUBDIRECTORY)    
    
    # deal with satan a little
    if( ${DM_STANDALONE} )
        set( CMAKE_INSTALL_PREFIX "" CACHE INTERNAL "" FORCE )
        mark_as_advanced( CMAKE_BUILD_TYPE )
    endif( ${DM_STANDALONE} )
    
    # some project internal variables
    set( DM_PROJECT_NAME ${LOCAL_PROJECT_NAME} )
    set( ${DM_PROJECT_NAME}_DEPENDENCIES "" )
    set( ${DM_PROJECT_NAME}_INTERNAL_HEADERS "" )
    set( ${DM_PROJECT_NAME}_INTERNAL_LIBRARIES "" )
    set( ${DM_PROJECT_NAME}_INTERNAL_EXECUTABLES "" )
    set( ${DM_PROJECT_NAME}_VERSION_MAJOR ${LOCAL_MAJOR_VERSION} )
    set( ${DM_PROJECT_NAME}_VERSION_MINOR ${LOCAL_MINOR_VERSION} )
    set( ${DM_PROJECT_NAME}_VERSION_REVISION ${LOCAL_REVISION_VERSION} )
    set( ${DM_PROJECT_NAME}_VERSION_FULL "${${DM_PROJECT_NAME}_VERSION_MAJOR}.${${DM_PROJECT_NAME}_VERSION_MINOR}.${${DM_PROJECT_NAME}_VERSION_REVISION}" )
    set( ${DM_PROJECT_NAME}_IDENTIFIER "${DM_PROJECT_NAME}.${${DM_PROJECT_NAME}_VERSION_FULL}" )

    # some advanced project structual cache variables
    set( ${DM_PROJECT_NAME}_HEADER_SUBDIRECTORY ${LOCAL_HEADER_SUBDIRECTORY} CACHE STRING "name of subdirectory in library subdirectories in which header files are kept" )
    set( ${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY ${LOCAL_SOURCE_SUBDIRECTORY} CACHE STRING "name of subdirectory in library subdirectories in which source are kept" )
    set( ${DM_PROJECT_NAME}_VERBOSE ON CACHE BOOL "report back to the user that everything is going ok" )
    mark_as_advanced( FORCE ${DM_PROJECT_NAME}_HEADER_SUBDIRECTORY ${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY ${DM_PROJECT_NAME}_VERBOSE )
    
    # some project installation cache variables
    set( ${DM_PROJECT_NAME}_SOURCE_DIRECTORY ${PROJECT_SOURCE_DIR} )
    if( ${DM_STANDALONE} )
        set( ${DM_PROJECT_NAME}_ROOT_DIRECTORY ${PROJECT_BINARY_DIR} )
    else( ${DM_STANDALONE} )
        if( ${DMAKE_BUILD} )
            set( ${DM_PROJECT_NAME}_ROOT_DIRECTORY "${DM_PARENT_PROJECT_NAME}_ROOT_DIRECTORY/${DM_PROJECT_NAME}" )
        else( ${DMAKE_BUILD} )
            set( ${DM_PROJECT_NAME}_ROOT_DIRECTORY "${CMAKE_INSTALL_PREFIX}/${DM_PROJECT_NAME}" )
        endif( ${DMAKE_BUILD} )
    endif( ${DM_STANDALONE} )
    if( ${${DM_PROJECT_NAME}_ROOT_DIRECTORY} STREQUAL ${PROJECT_SOURCE_DIR} )
        set( ${DM_PROJECT_NAME}_INSTALL_PREFIX "${${DM_PROJECT_NAME}_ROOT_DIRECTORY}/Install" CACHE PATH "${DM_PROJECT_NAME} installation prefix (changing this will recompute other install locations)" ) 
    else( ${${DM_PROJECT_NAME}_ROOT_DIRECTORY} STREQUAL ${PROJECT_SOURCE_DIR} )
        set( ${DM_PROJECT_NAME}_INSTALL_PREFIX ${${DM_PROJECT_NAME}_ROOT_DIRECTORY} CACHE PATH "${DM_PROJECT_NAME} installation prefix (changing this will recompute other install locations)" )
    endif( ${${DM_PROJECT_NAME}_ROOT_DIRECTORY} STREQUAL ${PROJECT_SOURCE_DIR} )   
    if( "${${DM_PROJECT_NAME}_INSTALL_PREFIX_INTERNAL}" STREQUAL "${${DM_PROJECT_NAME}_INSTALL_PREFIX}" )
        set( ${DM_PROJECT_NAME}_INSTALL_HEADERS_DIR "${${DM_PROJECT_NAME}_INSTALL_PREFIX}/include" CACHE PATH "${DM_PROJECT_NAME} installation directory for headers" )
        set( ${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR "${${DM_PROJECT_NAME}_INSTALL_PREFIX}/lib" CACHE PATH "${DM_PROJECT_NAME} installation directory for libraries" )
        set( ${DM_PROJECT_NAME}_INSTALL_EXECUTABLES_DIR "${${DM_PROJECT_NAME}_INSTALL_PREFIX}/bin" CACHE PATH "${DM_PROJECT_NAME} installation directory for libraries" )
    else( "${${DM_PROJECT_NAME}_INSTALL_PREFIX_INTERNAL}" STREQUAL "${${DM_PROJECT_NAME}_INSTALL_PREFIX}" )
        set( ${DM_PROJECT_NAME}_INSTALL_PREFIX_INTERNAL "${${DM_PROJECT_NAME}_INSTALL_PREFIX}" CACHE INTERNAL "" )
        set( ${DM_PROJECT_NAME}_INSTALL_HEADERS_DIR "${${DM_PROJECT_NAME}_INSTALL_PREFIX}/include" CACHE PATH "${DM_PROJECT_NAME} installation directory for headers" FORCE )
        set( ${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR "${${DM_PROJECT_NAME}_INSTALL_PREFIX}/lib" CACHE PATH "${DM_PROJECT_NAME} installation directory for libraries" FORCE )
        set( ${DM_PROJECT_NAME}_INSTALL_EXECUTABLES_DIR "${${DM_PROJECT_NAME}_INSTALL_PREFIX}/bin" CACHE PATH "${DM_PROJECT_NAME} installation directory for libraries" FORCE )
    endif( "${${DM_PROJECT_NAME}_INSTALL_PREFIX_INTERNAL}" STREQUAL "${${DM_PROJECT_NAME}_INSTALL_PREFIX}" )
    
    if( NOT DEFINED ${DM_PROJECT_NAME}_INSTALL_PREFIX_INTERNAL )
        set( ${DM_PROJECT_NAME}_INSTALL_PREFIX_INTERNAL "${${DM_PROJECT_NAME}_INSTALL_PREFIX}" CACHE INTERNAL "" )
    endif( NOT DEFINED ${DM_PROJECT_NAME}_INSTALL_PREFIX_INTERNAL ) 

    # add the automatically determined parts of the RPATH
    # which point to directories outside the build tree to the install RPATH
    set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )

    # the RPATH to be used when installing, but only if it's not a system directory
    list( FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR}" isSystemDir )
    if( "${isSystemDir}" STREQUAL "-1" )
        set( CMAKE_INSTALL_RPATH "${${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR}" )
    endif( "${isSystemDir}" STREQUAL "-1" )
    
endmacro()

macro( dmake_dependency_declare LOCAL_PROJECT_NAME LOCAL_MAJOR_VERSION LOCAL_MINOR_VERSION LOCAL_REVISION_VERSION )
    
    # import the dependency information
    include( ${DMAKE_CHACHE_DIR}/${LOCAL_PROJECT_NAME}.${LOCAL_MAJOR_VERSION}.${LOCAL_MINOR_VERSION}.${LOCAL_REVISION_VERSION}.cmake )

    # append the name of the dependency to the project dependency list
    list( APPEND ${DM_PROJECT_NAME}_DEPENDENCIES ${LOCAL_PROJECT_NAME} )    
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** declared dependency called <${LOCAL_PROJECT_NAME}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )    
    
endmacro()

macro( dmake_dependency_foreign_declare LOCAL_PROJECT_NAME LOCAL_PROJECT_INSTALL_HEADERS_DIR LOCAL_PROJECT_INSTALL_LIBRARIES_DIR )

    # append the name of the dependency to the project dependency list
    list( APPEND ${DM_PROJECT_NAME}_DEPENDENCIES ${LOCAL_PROJECT_NAME} )
    set( ${LOCAL_PROJECT_NAME}_INSTALL_HEADERS_DIR  "/usr/local/include" CACHE PATH "path to headers for <${LOCAL_PROJECT_NAME}>" )
    set( ${LOCAL_PROJECT_NAME}_INSTALL_LIBRARIES_DIR  "/usr/local/lib" CACHE PATH "path to libraries for <${LOCAL_PROJECT_NAME}>" )
    set( ${LOCAL_PROJECT_NAME}_LIBRARIES "" CACHE PATH "libraries for <${LOCAL_PROJECT_NAME}>" )
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** declared foreign dependency called <${LOCAL_PROJECT_NAME}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )    
    
endmacro()

# declare the existence of a library and the root subdirectory where its source code is kept
macro( dmake_library_declare LOCAL_LIBRARY_NAME LOCAL_LIBRARY_ROOT_DIRECTORY )
    
    # append the name of the library to the project library list
    list( APPEND ${DM_PROJECT_NAME}_INTERNAL_LIBRARIES ${LOCAL_LIBRARY_NAME} )
    
    # record root path of the library
    set( ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY ${${DM_PROJECT_NAME}_SOURCE_DIRECTORY}/${LOCAL_LIBRARY_ROOT_DIRECTORY} )
    
    # record header path of the library
    if( "${${DM_PROJECT_NAME}_HEADER_SUBDIRECTORY}" STREQUAL "" )
        set( ${LOCAL_LIBRARY_NAME}_HEADER_DIRECTORY ${${DM_PROJECT_NAME}_SOURCE_DIRECTORY} )
    else( "${${DM_PROJECT_NAME}_HEADER_SUBDIRECTORY}" STREQUAL "" )
        set( ${LOCAL_LIBRARY_NAME}_HEADER_DIRECTORY ${${DM_PROJECT_NAME}_SOURCE_DIRECTORY}/${${DM_PROJECT_NAME}_HEADER_SUBDIRECTORY} )
    endif( "${${DM_PROJECT_NAME}_HEADER_SUBDIRECTORY}" STREQUAL "" )
    
    # record source path of the library
    if( "${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY}" STREQUAL "" )
        set( ${LOCAL_LIBRARY_NAME}_SOURCE_DIRECTORY ${${DM_PROJECT_NAME}_SOURCE_DIRECTORY} )
    else( "${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY}" STREQUAL "" )
        set( ${LOCAL_LIBRARY_NAME}_SOURCE_DIRECTORY ${${DM_PROJECT_NAME}_SOURCE_DIRECTORY}/${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY} )
    endif( "${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY}" STREQUAL "" )
    
    # add some editable cache variables controlling build behavior
    set( ${LOCAL_LIBRARY_NAME}_SHARED ON CACHE BOOL "enable or disable shared nature of library <${LOCAL_LIBRARY_NAME}>" )
    set( ${LOCAL_LIBRARY_NAME}_ENABLED ON CACHE BOOL "enable or disable building of library <${LOCAL_LIBRARY_NAME}>" )
    mark_as_advanced( ${LOCAL_LIBRARY_NAME}_SHARED ${LOCAL_LIBRARY_NAME}_ENABLED )
    
    # clear out headers and source lists for the library
    set( ${LOCAL_LIBRARY_NAME}_HEADERS "" )
    set( ${LOCAL_LIBRARY_NAME}_SOURCES "" )
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** declared a library called <${LOCAL_LIBRARY_NAME}> in subdirectory <${${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
    
endmacro()

# add header content to a library (MUST be previously declared!)
macro( dmake_library_headers LOCAL_LIBRARY_NAME )

    # make sure the library was defined
    if( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME} has not been defined!" )
    endif( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )

    # add all headers given to the list of headers for this library
    foreach( HEADER_FILE ${ARGN} )
        list( APPEND ${LOCAL_LIBRARY_NAME}_HEADERS ${${LOCAL_LIBRARY_NAME}_HEADER_DIRECTORY}/${HEADER_FILE} )
    endforeach()
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME}> has headers <${${LOCAL_LIBRARY_NAME}_HEADERS}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
    
endmacro()

# add header content to a library (MUST be previously declared!)
macro( dmake_library_headers_external LOCAL_LIBRARY_NAME )

    # make sure the library was defined
    if( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME} has not been defined!" )
    endif( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )

    # add all headers given to the list of headers for this library
    foreach( HEADER_FILE ${ARGN} )
        list( APPEND ${LOCAL_LIBRARY_NAME}_HEADERS ${HEADER_FILE} )
    endforeach()
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME}> has headers <${${LOCAL_LIBRARY_NAME}_HEADERS}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
    
endmacro()

# add source content to a library (MUST be previously declared!)
macro( dmake_library_sources LOCAL_LIBRARY_NAME )

    # make sure the library was defined
    if( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME} has not been defined!" )
    endif( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )

    # add all given sources to the list of sources for this library
    foreach( SOURCE_FILE ${ARGN} )
        list( APPEND ${LOCAL_LIBRARY_NAME}_SOURCES ${${LOCAL_LIBRARY_NAME}_SOURCE_DIRECTORY}/${SOURCE_FILE} )
    endforeach()
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME}> has sources <${${LOCAL_LIBRARY_NAME}_SOURCES}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )

endmacro()

# add source content to a library (MUST be previously declared!)
macro( dmake_library_sources_external LOCAL_LIBRARY_NAME )

    # make sure the library was defined
    if( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME} has not been defined!" )
    endif( NOT DEFINED ${LOCAL_LIBRARY_NAME}_ROOT_DIRECTORY )

    # add all given sources to the list of sources for this library
    foreach( SOURCE_FILE ${ARGN} )
        list( APPEND ${LOCAL_LIBRARY_NAME}_SOURCES ${SOURCE_FILE} )
    endforeach()
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** library called <${LOCAL_LIBRARY_NAME}> has sources <${${LOCAL_LIBRARY_NAME}_SOURCES}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )

endmacro()

# declare the existence of an executable and the root subdirectory where its sources are kept
macro( dmake_executable_declare LOCAL_EXECUTABLE_NAME LOCAL_EXECUTABLE_ROOT_DIRECTORY )
    
    # append the name of the executable to the project executable list
    list( APPEND ${DM_PROJECT_NAME}_INTERNAL_EXECUTABLES ${LOCAL_EXECUTABLE_NAME} )
    
    # record root path of the executable
    set( ${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY ${${DM_PROJECT_NAME}_SOURCE_DIRECTORY}/${LOCAL_EXECUTABLE_ROOT_DIRECTORY} )
    
    # record source path of the executable
    if( "${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY}" STREQUAL "" )
        set( ${LOCAL_EXECUTABLE_NAME}_SOURCE_DIRECTORY ${${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY} )
    else( "${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY}" STREQUAL "" )
        set( ${LOCAL_EXECUTABLE_NAME}_SOURCE_DIRECTORY ${${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY}/${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY} )
    endif( "${${DM_PROJECT_NAME}_SOURCE_SUBDIRECTORY}" STREQUAL "" )
    
    # add some editable cache variables controlling build behavior
    set( ${LOCAL_EXECUTABLE_NAME}_ENABLED ON CACHE BOOL "enable or disable building of executable <${LOCAL_EXECUTABLE_NAME}>" )
    mark_as_advanced( ${LOCAL_EXECUTABLE_NAME}_ENABLED )

    # clear out sources for the executable    
    set( ${LOCAL_EXECUTABLE_NAME}_SOURCES "" )
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** declared an executable called <${LOCAL_EXECUTABLE_NAME}> at <${${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
    
endmacro()

# add source content to an executable (MUST be previously declared!)
macro( dmake_executable_sources LOCAL_EXECUTABLE_NAME )

    # make sure the executable was defined
    if( NOT DEFINED ${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY )
        message( STATUS "*** executable called <${LOCAL_EXECUTABLE_NAME} has not been defined!" )
    endif( NOT DEFINED ${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY )

    # add all source given to the list of sources for this executable
    foreach( SOURCE_FILE ${ARGN} )
        list( APPEND ${LOCAL_EXECUTABLE_NAME}_SOURCES ${${LOCAL_EXECUTABLE_NAME}_SOURCE_DIRECTORY}/${SOURCE_FILE} )
    endforeach()
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** executable called <${LOCAL_EXECUTABLE_NAME}> has sources <${${LOCAL_EXECUTABLE_NAME}_SOURCES}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )

endmacro()

# add source content to an executable (MUST be previously declared!)
macro( dmake_executable_sources_external LOCAL_EXECUTABLE_NAME )

    # make sure the executable was defined
    if( NOT DEFINED ${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY )
        message( STATUS "*** executable called <${LOCAL_EXECUTABLE_NAME} has not been defined!" )
    endif( NOT DEFINED ${LOCAL_EXECUTABLE_NAME}_ROOT_DIRECTORY )

    # add all source given to the list of sources for this executable
    foreach( SOURCE_FILE ${ARGN} )
        list( APPEND ${LOCAL_EXECUTABLE_NAME}_SOURCES ${SOURCE_FILE} )
    endforeach()
    
    # talk about our feelings
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** executable called <${LOCAL_EXECUTABLE_NAME}> has sources <${${LOCAL_EXECUTABLE_NAME}_SOURCES}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )

endmacro()

# trigger everything and finish
macro( dmake_project_end )
    
    # add all external includes to the compiler include path
    set( EXTERNAL_INCLUDES "" )
    foreach( DEPENDENCY_NAME ${${DM_PROJECT_NAME}_DEPENDENCIES} )
        list( APPEND EXTERNAL_INCLUDES "${${DEPENDENCY_NAME}_INSTALL_HEADERS_DIR}" )
    endforeach()
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** external includes are <${EXTERNAL_INCLUDES}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
    
    # add all external libraries to the linker link path
    foreach( DEPENDENCY_NAME ${${DM_PROJECT_NAME}_DEPENDENCIES} )
        foreach( LIBRARY_NAME ${${DEPENDENCY_NAME}_LIBRARIES} )
            list( APPEND EXTERNAL_LIBRARIES "${${DEPENDENCY_NAME}_INSTALL_LIBRARIES_DIR}/lib${LIBRARY_NAME}.so" )
        endforeach()
    endforeach()
    if( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
        message( STATUS "*** external libraries are <${EXTERNAL_LIBRARIES}>" )
    endif( "${${DM_PROJECT_NAME}_VERBOSE}" STREQUAL "ON" )
    
    
    # tell satan where the includes are
    foreach( LIBRARY_NAME ${${DM_PROJECT_NAME}_INTERNAL_LIBRARIES} )
        include_directories( "${${LIBRARY_NAME}_HEADER_DIRECTORY}" )
        foreach( HEADER_NAME ${${LIBRARY_NAME}_HEADERS} )
            list( APPEND ${DM_PROJECT_NAME}_INTERNAL_HEADERS ${HEADER_NAME} )
        endforeach()
    endforeach()
    include_directories( ${EXTERNAL_INCLUDES} ${INTERNAL_INCLUDES} )

    # tell satan to add all libraries to the build
    foreach( LIBRARY_NAME ${${DM_PROJECT_NAME}_INTERNAL_LIBRARIES} )
        if( "${${LIBRARY_NAME}_ENABLED}" STREQUAL "ON" )
            if( "${${LIBRARY_NAME}_SHARED}" STREQUAL "ON" )
                add_library( ${LIBRARY_NAME} SHARED ${${LIBRARY_NAME}_SOURCES} )
                set_target_properties( ${LIBRARY_NAME} PROPERTIES INSTALL_NAME_DIR ${${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR})
            else( "${${LIBRARY_NAME}_SHARED}" STREQUAL "ON" )
                add_library( ${LIBRARY_NAME} STATIC ${${LIBRARY_NAME}_SOURCES} )
            endif( "${${LIBRARY_NAME}_SHARED}" STREQUAL "ON" )
            target_link_libraries( ${LIBRARY_NAME} ${EXTERNAL_LIBRARIES} )
#	    export( TARGETS ${LIBRARY_NAME} APPEND FILE "MonarchExport.cmake" )
        endif( "${${LIBRARY_NAME}_ENABLED}" STREQUAL "ON" )
    endforeach()
    
    # tell satan to add all executables to the build
    foreach( EXECUTABLE_NAME ${${DM_PROJECT_NAME}_INTERNAL_EXECUTABLES} )
        if( "${${EXECUTABLE_NAME}_ENABLED}" STREQUAL "ON" )
            add_executable( ${EXECUTABLE_NAME} ${${EXECUTABLE_NAME}_SOURCES} )
            foreach( LIBRARY_NAME ${${DM_PROJECT_NAME}_INTERNAL_LIBRARIES} )
                if( "${${LIBRARY_NAME}_ENABLED}" STREQUAL "ON" )
                    target_link_libraries( ${EXECUTABLE_NAME} ${LIBRARY_NAME} )
                endif( "${${LIBRARY_NAME}_ENABLED}" STREQUAL "ON" )
            endforeach()
            target_link_libraries( ${EXECUTABLE_NAME} ${EXTERNAL_LIBRARIES} )
#	    export( TARGETS ${EXECUTABLE_NAME} APPEND FILE "MonarchExport.cmake" )
        endif( "${${EXECUTABLE_NAME}_ENABLED}" STREQUAL "ON" )
    endforeach()
    
    # prepare the cache file
    if( ${DMAKE_USE_CACHE} )
        set( PACKAGE_CACHE_FILE ${DMAKE_CACHE_DIR}/${${DM_PROJECT_NAME}_IDENTIFIER}.cmake )
    else( ${DMAKE_USE_CACHE} )
        set( PACKAGE_CACHE_FILE ${${DM_PROJECT_NAME}_INSTALL_PREFIX}/${${DM_PROJECT_NAME}_IDENTIFIER}.cmake )
    endif( ${DMAKE_USE_CACHE} )
    file( WRITE  ${PACKAGE_CACHE_FILE} "\# this file was automatically generated by dmake to describe an installation of <${DM_PROJECT_NAME}>\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_VERSION_MAJOR ${${DM_PROJECT_NAME}_VERSION_MAJOR} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_VERSION_MINOR ${${DM_PROJECT_NAME}_VERSION_MINOR} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_VERSION_REVISION ${${DM_PROJECT_NAME}_VERSION_REVISION} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_VERSION_FULL ${${DM_PROJECT_NAME}_VERSION_FULL} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_IDENTIFIER ${${DM_PROJECT_NAME}_IDENTIFIER} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_INSTALL_HEADERS_DIR ${${DM_PROJECT_NAME}_INSTALL_HEADERS_DIR} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR ${${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_INSTALL_EXECUTABLES_DIR ${${DM_PROJECT_NAME}_INSTALL_EXECUTABLES_DIR} )\n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_INTERNAL_LIBRARIES ${${DM_PROJECT_NAME}_INTERNAL_LIBRARIES} ) \n" )
    file( APPEND ${PACKAGE_CACHE_FILE} "set( ${DM_PROJECT_NAME}_INTERNAL_EXECUTABLES ${${DM_PROJECT_NAME}_INTERNAL_EXECUTABLES} )\n" )
    
    #install everything
    install( FILES ${${DM_PROJECT_NAME}_INTERNAL_HEADERS} DESTINATION ${${DM_PROJECT_NAME}_INSTALL_HEADERS_DIR} )
    install( TARGETS ${${DM_PROJECT_NAME}_INTERNAL_LIBRARIES} DESTINATION ${${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR} )
    install( TARGETS ${${DM_PROJECT_NAME}_INTERNAL_EXECUTABLES} DESTINATION ${${DM_PROJECT_NAME}_INSTALL_EXECUTABLES_DIR} )
    
    if( NOT ${DM_STANDALONE} )
        set( ${DM_PROJECT_NAME}_LIBRARIES ${${DM_PROJECT_NAME}_INTERNAL_LIBRARIES} PARENT_SCOPE )
        set( ${DM_PROJECT_NAME}_LIBRARY_DIR ${${DM_PROJECT_NAME}_INSTALL_LIBRARIES_DIR} PARENT_SCOPE )
        set( ${DM_PROJECT_NAME}_INCLUDE_DIR ${${DM_PROJECT_NAME}_INSTALL_HEADERS_DIR} PARENT_SCOPE )
    endif( NOT ${DM_STANDALONE} )
    
endmacro()

# bye
