cmake_minimum_required(VERSION 3.8)
project(ca_structure)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set( DEPENDENCIES
  ca_msgs
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  cs4home_core
)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(ament_cmake REQUIRED)
foreach(pkg ${DEPENDENCIES})
  find_package(${pkg} REQUIRED)
endforeach()

find_package(yaml-cpp REQUIRED)


include_directories(
  include
  include/${PROJECT_NAME}
)

add_library(${PROJECT_NAME} SHARED
  src/ca_gateway.cpp
  src/ca_gateway_client.cpp
  src/ca_gateway_client_afferent.cpp
)
ament_target_dependencies(${PROJECT_NAME} ${DEPENDENCIES})
target_link_libraries(${PROJECT_NAME} yaml-cpp)

add_executable(ca_gateway_node src/ca_gateway_node.cpp)
target_link_libraries(ca_gateway_node ${PROJECT_NAME})
ament_target_dependencies(ca_gateway_node ${DEPENDENCIES})

install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_dependencies(${DEPENDENCIES})

install(TARGETS ca_gateway_node
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()

  add_subdirectory(tests)
endif()

ament_package()
