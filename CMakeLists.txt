cmake_minimum_required(VERSION 3.2)
project (dvbtest)

set(CMAKE_CXX_STANDARD 17)

add_executable(dvbtest Util.cpp Transponder.cpp DVBTable.cpp ProgramAssociationTable.cpp ProgramMapTable.cpp ServiceDescriptionTable.cpp DVBInterface.cpp DVBDescriptor.cpp ISO639_2.cpp main.cpp)
