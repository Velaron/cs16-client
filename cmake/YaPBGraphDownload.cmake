# Notify of an attempt to download graphs
message("-- Starting downloading the YaPB Graphs")

# Specify the YaPB Graph Database URL
set(DATABASE_URL "https://yapb.jeefo.net/graph/")

# Specify the path to the file with the maps list
set(LIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/official_maps.lst")

# Checking the presence of the list file
if (NOT EXISTS "${LIST_FILE}")
    message(FATAL_ERROR "File ${LIST_FILE} not found.")
endif()

# Read the map list line by line and download graphs for them
file(STRINGS "${LIST_FILE}" MAP_NAMES)
foreach(MAP_NAME ${MAP_NAMES})
    set(FILE_URL "${DATABASE_URL}${MAP_NAME}.graph")
    file(DOWNLOAD "${FILE_URL}" "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/cs16client-extras/addons/yapb/data/graph/${MAP_NAME}.graph")
    message("-- Downloading ${MAP_NAME}.graph")
endforeach()