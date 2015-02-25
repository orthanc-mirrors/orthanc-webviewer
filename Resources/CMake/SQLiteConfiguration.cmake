# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


if (STATIC_BUILD OR NOT USE_SYSTEM_SQLITE)
  SET(SQLITE_SOURCES_DIR ${CMAKE_BINARY_DIR}/sqlite-amalgamation-3071300)
  DownloadPackage(
    "5fbeff9645ab035a1f580e90b279a16d"
    "http://www.montefiore.ulg.ac.be/~jodogne/Orthanc/ThirdPartyDownloads/sqlite-amalgamation-3071300.zip"
    "${SQLITE_SOURCES_DIR}")

  list(APPEND SQLITE_SOURCES
    ${SQLITE_SOURCES_DIR}/sqlite3.c
    )

  add_definitions(
    # For SQLite to run in the "Serialized" thread-safe mode
    # http://www.sqlite.org/threadsafe.html
    -DSQLITE_THREADSAFE=1  
    -DSQLITE_OMIT_LOAD_EXTENSION  # Disable SQLite plugins
    )

  include_directories(
    ${SQLITE_SOURCES_DIR}
    )

  source_group(ThirdParty\\SQLite REGULAR_EXPRESSION ${SQLITE_SOURCES_DIR}/.*)

else()
  CHECK_INCLUDE_FILE_CXX(sqlite3.h HAVE_SQLITE_H)
  if (NOT HAVE_SQLITE_H)
    message(FATAL_ERROR "Please install the libsqlite3-dev package")
  endif()

  # Autodetection of the version of SQLite
  file(STRINGS "/usr/include/sqlite3.h" SQLITE_VERSION_NUMBER1 REGEX "#define SQLITE_VERSION_NUMBER.*$")    
  string(REGEX REPLACE "#define SQLITE_VERSION_NUMBER(.*)$" "\\1" SQLITE_VERSION_NUMBER ${SQLITE_VERSION_NUMBER1})

  message("Detected version of SQLite: ${SQLITE_VERSION_NUMBER}")

  IF (${SQLITE_VERSION_NUMBER} LESS 3007000)
    # "sqlite3_create_function_v2" is not defined in SQLite < 3.7.0
    message(FATAL_ERROR "SQLite version must be above 3.7.0. Please set the CMake variable USE_SYSTEM_SQLITE to OFF.")
  ENDIF()

  link_libraries(sqlite3)
endif()
