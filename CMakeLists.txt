cmake_minimum_required(VERSION 3.22)

project(TerraVoxelHttpServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Core
    Gui
    Network
    Sql
    HttpServer
    Concurrent
)

qt_add_executable(TerraVoxelHttpServer
    main.cpp
    entity.h
    entity.cpp
    france.h
    france.cpp
    region.h
    region.cpp
    departement.h
    departement.cpp
    epci.h
    epci.cpp
    commune.h
    commune.cpp
    repository.h
    repository.cpp
    httpserver.h
    httpserver.cpp
    mesh.h
    mesh.cpp
    meshserializer.h
    meshserializer.cpp
    meshcache.h
    meshcache.cpp
    rastercache.h
    rastercache.cpp
    wkbreader.h
    wkbreader.cpp
    codelist.h
    codelist.cpp
    tuile.h
    tuile.cpp
    tilepyramid.h
    tilepyramid.cpp
    tileset.h
    tileset.cpp
    tilecoverage.h
    tilecache.h
    tilecache.cpp
    tilecoveragecalculator.h
    tilecoveragecalculator.cpp
)

target_link_libraries(TerraVoxelHttpServer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Sql
    Qt6::HttpServer
    Qt6::Concurrent
)

target_compile_definitions(TerraVoxelHttpServer PRIVATE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
)

install(TARGETS TerraVoxelHttpServer
    RUNTIME DESTINATION bin
)
