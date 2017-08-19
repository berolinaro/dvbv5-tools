cmake_minimum_required(VERSION 3.2)
project (dvbtest)

set(CMAKE_CXX_STANDARD 17)

add_library(dvbtools Util.cpp Transponder.cpp DVBTable.cpp ProgramAssociationTable.cpp ProgramMapTable.cpp ServiceDescriptionTable.cpp NetworkInformationTable.cpp DVBInterface.cpp DVBDescriptor.cpp ISO639_2.cpp Service.cpp ChannelList.cpp Lnb.cpp)
link_libraries(dvbtools)
add_executable(dvbtest dvbtest.cpp)
add_executable(scan scan.cpp)
add_executable(watch watch.cpp)
add_executable(stream stream.cpp)
target_link_libraries(stream pthread)
