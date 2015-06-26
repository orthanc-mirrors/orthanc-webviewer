set(AUTOGENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/AUTOGENERATED")
set(AUTOGENERATED_SOURCES)

file(MAKE_DIRECTORY ${AUTOGENERATED_DIR})
include_directories(${AUTOGENERATED_DIR})

macro(EmbedResources)
  # Convert a semicolon separated list to a whitespace separated string
  set(SCRIPT_ARGUMENTS)
  set(DEPENDENCIES)
  set(IS_PATH_NAME false)
  foreach(arg ${ARGN})
    if (${IS_PATH_NAME})
      list(APPEND SCRIPT_ARGUMENTS "${arg}")
      list(APPEND DEPENDENCIES "${arg}")
      set(IS_PATH_NAME false)
    else()
      list(APPEND SCRIPT_ARGUMENTS "${arg}")
      set(IS_PATH_NAME true)
    endif()
  endforeach()

  set(TARGET_BASE "${AUTOGENERATED_DIR}/EmbeddedResources")
  add_custom_command(
    OUTPUT
    "${TARGET_BASE}.h"
    "${TARGET_BASE}.cpp"
    COMMAND 
    ${PYTHON_EXECUTABLE}
    "${ORTHANC_ROOT}/Resources/EmbedResources.py"
    "${AUTOGENERATED_DIR}/EmbeddedResources"
    ${SCRIPT_ARGUMENTS}
    DEPENDS
    "${ORTHANC_ROOT}/Resources/EmbedResources.py"
    ${DEPENDENCIES}
    )

  list(APPEND AUTOGENERATED_SOURCES
    "${AUTOGENERATED_DIR}/EmbeddedResources.cpp"
    ) 
endmacro()