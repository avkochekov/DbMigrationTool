cmake_minimum_required(VERSION 3.14)

project(DbMigrationTool LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

add_library(DbMigrationTool SHARED
  DbMigrationTool_global.h
  db_migration_tool.cpp
  db_migration_tool.h
  db_version.h
  db_migration_exception.h db_migration_exception.cpp
)

target_link_libraries(DbMigrationTool PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(DbMigrationTool PRIVATE Qt${QT_VERSION_MAJOR}::Sql)

target_compile_definitions(DbMigrationTool PRIVATE DBMIGRATIONTOOL_LIBRARY)
