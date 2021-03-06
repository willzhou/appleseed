
#
# This source file is part of appleseed.
# Visit http://appleseedhq.net/ for additional information and resources.
#
# This software is released under the MIT license.
#
# Copyright (c) 2010-2012 Francois Beaune, Jupiter Jazz
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#


#--------------------------------------------------------------------------------------------------
# Source files.
#--------------------------------------------------------------------------------------------------

set (sources
    commandlinehandler.cpp
    commandlinehandler.h
    main.cpp
)
list (APPEND normalizeprojectfile_sources
    ${sources}
)
source_group ("" FILES
    ${sources}
)


#--------------------------------------------------------------------------------------------------
# Target.
#--------------------------------------------------------------------------------------------------

add_executable (normalizeprojectfile
    ${normalizeprojectfile_sources}
)

set_target_properties (normalizeprojectfile PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${platform}/normalizeprojectfile
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${platform}/normalizeprojectfile
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../build/${platform}/normalizeprojectfile
)


#--------------------------------------------------------------------------------------------------
# Include paths.
#--------------------------------------------------------------------------------------------------

include_directories (
    .
    ../../appleseed.shared
)


#--------------------------------------------------------------------------------------------------
# Preprocessor definitions.
#--------------------------------------------------------------------------------------------------

apply_preprocessor_definitions (normalizeprojectfile)


#--------------------------------------------------------------------------------------------------
# Static libraries.
#--------------------------------------------------------------------------------------------------

link_against_platform (normalizeprojectfile)

target_link_libraries (normalizeprojectfile
    appleseed
    appleseed.shared
    ${Boost_LIBRARIES}
)


#--------------------------------------------------------------------------------------------------
# Post-build commands.
#--------------------------------------------------------------------------------------------------

add_copy_target_to_sandbox_command (normalizeprojectfile)
