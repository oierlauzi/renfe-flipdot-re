cmake_minimum_required(VERSION 3.16)

# IDF_PATH may come from the environment (after sourcing export.sh) or from the
# command line, so the tree can be configured without touching the shell:
#
#   cmake -DIDF_PATH=/home/oier/esp/esp-idf -B build -G Ninja
#
# The CLI value wins only when the environment does not already carry one, so a
# sourced shell stays authoritative and the two cannot silently disagree.
if(NOT DEFINED ENV{IDF_PATH})
    if(DEFINED IDF_PATH)
        set(ENV{IDF_PATH} "${IDF_PATH}")
    else()
        message(FATAL_ERROR
            "IDF_PATH is not set. Either source the export script:\n"
            "    . /home/oier/esp/esp-idf/export.sh\n"
            "or pass it on the command line:\n"
            "    cmake -DIDF_PATH=/home/oier/esp/esp-idf ...\n"
            "Note that idf.py still needs the toolchain on PATH, which only "
            "export.sh provides.")
    endif()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(flipdot-controller)
