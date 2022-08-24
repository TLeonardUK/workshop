# ================================================================================================
#  workshop
#  Copyright (C) 2022 Tim Leonard
# ================================================================================================
project(yamlcpp C CXX)

SET(SOURCES
    "src/binary.cpp"
    "src/collectionstack.h"
    "src/convert.cpp"
    "src/depthguard.cpp"
    "src/directives.cpp"
    "src/directives.h"
    "src/emit.cpp"
    "src/emitfromevents.cpp"
    "src/emitter.cpp"
    "src/emitterstate.cpp"
    "src/emitterstate.h"
    "src/emitterutils.cpp"
    "src/emitterutils.h"
    "src/exceptions.cpp"
    "src/exp.cpp"
    "src/exp.h"
    "src/indentation.h"
    "src/memory.cpp"
    "src/node.cpp"
    "src/nodebuilder.cpp"
    "src/nodebuilder.h"
    "src/nodeevents.cpp"
    "src/nodeevents.h"
    "src/node_data.cpp"
    "src/null.cpp"
    "src/ostream_wrapper.cpp"
    "src/parse.cpp"
    "src/parser.cpp"
    "src/ptr_vector.h"
    "src/regeximpl.h"
    "src/regex_yaml.cpp"
    "src/regex_yaml.h"
    "src/scanner.cpp"
    "src/scanner.h"
    "src/scanscalar.cpp"
    "src/scanscalar.h"
    "src/scantag.cpp"
    "src/scantag.h"
    "src/scantoken.cpp"
    "src/setting.h"
    "src/simplekey.cpp"
    "src/singledocparser.cpp"
    "src/singledocparser.h"
    "src/stream.cpp"
    "src/stream.h"
    "src/streamcharsource.h"
    "src/stringsource.h"
    "src/tag.cpp"
    "src/tag.h"
    "src/token.h"
)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

util_setup_folder_structure(${PROJECT_NAME} SOURCES "engine/thirdparty")


