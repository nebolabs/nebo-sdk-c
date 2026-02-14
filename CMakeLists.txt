cmake_minimum_required(VERSION 3.16)
project(nebo-sdk-c C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ── Dependencies ──────────────────────────────────────────────────────

# Let gRPC pull in protobuf as its own dependency to avoid version conflicts
find_package(gRPC CONFIG REQUIRED)

# Locate plugins
find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# ── Proto compilation ─────────────────────────────────────────────────

set(PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/proto)
set(PROTO_GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

set(PROTO_FILES
    ${PROTO_DIR}/apps/v0/common.proto
    ${PROTO_DIR}/apps/v0/tool.proto
    ${PROTO_DIR}/apps/v0/channel.proto
    ${PROTO_DIR}/apps/v0/gateway.proto
    ${PROTO_DIR}/apps/v0/ui.proto
    ${PROTO_DIR}/apps/v0/comm.proto
    ${PROTO_DIR}/apps/v0/schedule.proto
)

set(PROTO_SRCS)
set(PROTO_HDRS)

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(PB_CC  ${PROTO_GEN_DIR}/proto/apps/v0/${PROTO_NAME}.pb.cc)
    set(PB_H   ${PROTO_GEN_DIR}/proto/apps/v0/${PROTO_NAME}.pb.h)
    set(GRPC_CC ${PROTO_GEN_DIR}/proto/apps/v0/${PROTO_NAME}.grpc.pb.cc)
    set(GRPC_H  ${PROTO_GEN_DIR}/proto/apps/v0/${PROTO_NAME}.grpc.pb.h)

    add_custom_command(
        OUTPUT ${PB_CC} ${PB_H} ${GRPC_CC} ${GRPC_H}
        COMMAND ${PROTOC}
            --proto_path=${PROTO_ROOT}
            --cpp_out=${PROTO_GEN_DIR}
            --grpc_out=${PROTO_GEN_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating C++ protos for ${PROTO_NAME}"
    )

    list(APPEND PROTO_SRCS ${PB_CC} ${GRPC_CC})
    list(APPEND PROTO_HDRS ${PB_H} ${GRPC_H})
endforeach()

# ── SDK library ───────────────────────────────────────────────────────

add_library(nebo-sdk
    src/nebo.c
    src/schema.c
    src/view.c
    src/grpc_server.cc
    ${PROTO_SRCS}
)

target_include_directories(nebo-sdk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(nebo-sdk PRIVATE
    ${PROTO_GEN_DIR}
)

target_link_libraries(nebo-sdk
    gRPC::grpc++
    protobuf::libprotobuf
)

# ── Calculator example ────────────────────────────────────────────────

add_executable(calculator examples/calculator/main.c)
target_link_libraries(calculator nebo-sdk)
