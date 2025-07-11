CMAKE_MINIMUM_REQUIRED(VERSION 3.8)
project(rd_utils)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g -std=c++17 -Werror=all")

file(
  GLOB_RECURSE
  SRC
  src/*.cc
  )


file(
  GLOB_RECURSE
  SRC_C
  src/*.c
  )

file(
  GLOB
  HEADER_CONCURRENCY
  src/rd_utils/concurrency/*.hh
  )

file(
  GLOB
  HEADER_ACTOR
  src/rd_utils/concurrency/actor/*.hh
  )

file(
  GLOB
  HEADER_NET
  src/rd_utils/net/*.hh
  )

file(
  GLOB
  HEADER_UTILS
  src/rd_utils/utils/*.hh
  )

file(
  GLOB
  HEADER_FOREIGN
  src/rd_utils/foreign/*.hh
  )


file(
  GLOB
  HEADER_MEMORY
  src/rd_utils/memory/*.hh
  )

file(
  GLOB
  HEADER_MEMORY_CACHE
  src/rd_utils/memory/cache/*.hh
  )


file(
  GLOB
  HEADER_MEMORY_CACHE_COLL
  src/rd_utils/memory/cache/collection/*.hh
  )
  
file(
  GLOB
  HEADER_MEMORY_CACHE_ALGOS
  src/rd_utils/memory/cache/algorithm/*.hh
  )

file(
  GLOB
  HEADER_MEMORY_CACHE_REPO
  src/rd_utils/memory/cache/remote/*.hh
  )

file(
  GLOB
  HEADER_MEMORY_GC
  src/rd_utils/memory/gc/*.hh
  )


file(
  GLOB
  HEADER_ROOT
  src/rd_utils/*.hh
  )


file(
  GLOB
  HEADER_CONFIG
  src/rd_utils/utils/config/*.hh
  )

file(
  GLOB
  HEADER_TRACE
  src/rd_utils/utils/trace/*.hh
  )

find_package(nlohmann_json 3.7.3 REQUIRED)
add_library (rd_utils ${SRC} ${SRC_C})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

install (TARGETS rd_utils DESTINATION /usr/lib/)

install (FILES ${HEADER_CONCURRENCY} DESTINATION /usr/include/rd_utils/concurrency)
install (FILES ${HEADER_ACTOR} DESTINATION /usr/include/rd_utils/concurrency/actor)
install (FILES ${HEADER_NET} DESTINATION /usr/include/rd_utils/net)
install (FILES ${HEADER_UTILS} DESTINATION /usr/include/rd_utils/utils)
install (FILES ${HEADER_FOREIGN} DESTINATION /usr/include/rd_utils/foreign)
install (FILES ${HEADER_MEMORY} DESTINATION /usr/include/rd_utils/memory)
install (FILES ${HEADER_MEMORY_GC} DESTINATION /usr/include/rd_utils/memory/gc)
install (FILES ${HEADER_MEMORY_CACHE} DESTINATION /usr/include/rd_utils/memory/cache)
install (FILES ${HEADER_MEMORY_CACHE_COLL} DESTINATION /usr/include/rd_utils/memory/cache/collection)
install (FILES ${HEADER_MEMORY_CACHE_ALGOS} DESTINATION /usr/include/rd_utils/memory/cache/algorithm)
install (FILES ${HEADER_MEMORY_CACHE_REPO} DESTINATION /usr/include/rd_utils/memory/cache/remote)
install (FILES ${HEADER_CONFIG} DESTINATION /usr/include/rd_utils/utils/config)
install (FILES ${HEADER_TRACE} DESTINATION /usr/include/rd_utils/utils/trace)
install (FILES ${HEADER_ROOT} DESTINATION /usr/include/rd_utils/)


target_link_libraries (rd_utils nlohmann_json::nlohmann_json ssl crypto ssh)
