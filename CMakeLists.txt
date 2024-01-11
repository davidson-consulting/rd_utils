CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
project(rd_utils)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++17")

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
  HEADER_ROOT
  src/rd_utils/*.hh
  )

find_package(nlohmann_json 3.7.3 REQUIRED)
add_library (rd_utils ${SRC} ${SRC_C})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

install (TARGETS rd_utils DESTINATION /usr/lib/)

install (FILES ${HEADER_CONCURRENCY} DESTINATION /usr/include/rd_utils/concurrency)
install (FILES ${HEADER_NET} DESTINATION /usr/include/rd_utils/net)
install (FILES ${HEADER_UTILS} DESTINATION /usr/include/rd_utils/utils)
install (FILES ${HEADER_FOREIGN} DESTINATION /usr/include/rd_utils/foreign)
install (FILES ${HEADER_MEMORY} DESTINATION /usr/include/rd_utils/memory)
install (FILES ${HEADER_ROOT} DESTINATION /usr/include/rd_utils/)


target_link_libraries (rd_utils dio_common nlohmann_json::nlohmann_json ssl crypto ssh)
