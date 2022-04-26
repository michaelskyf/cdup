cmake_minimum_required(VERSION 3.23)

project(cdup)

# C++
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# C
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

add_subdirectory(src)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(
	"${PROJECT_NAME}"
	${sources}
	)

find_package(OpenSSL REQUIRED)

target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
