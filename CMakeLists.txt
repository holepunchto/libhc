cmake_minimum_required(VERSION 3.25)

find_package(cmake-bare REQUIRED PATHS node_modules/cmake-bare)
find_package(cmake-fetch REQUIRED PATHS node_modules/cmake-fetch)

project(hc C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

fetch_package("github:jedisct1/libsodium#e18eee6" SOURCE_DIR sodium)

# Pinned to the `sync` branch from holepunchto/librocksdb PR #28 while the
# synchronous API stabilises. Move back to the default ref once it merges.
fetch_package("github:holepunchto/librocksdb#sync")

fetch_package("github:holepunchto/liblexkey")

fetch_package("github:holepunchto/libflattree")

bare_target(target)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(COPY_FILE
  "${sodium}/builds/msvc/version.h"
  "${sodium}/src/libsodium/include/sodium/version.h"
)

file(GLOB_RECURSE sodium_headers CONFIGURE_DEPENDS "${sodium}/src/libsodium/**/*.h")
file(GLOB_RECURSE sodium_sources CONFIGURE_DEPENDS "${sodium}/src/libsodium/**/*.c")
file(GLOB_RECURSE sodium_asm_sources CONFIGURE_DEPENDS "${sodium}/src/libsodium/**/*.S")

add_library(sodium OBJECT)

target_sources(
  sodium
  INTERFACE
    ${sodium_headers}
  PRIVATE
    ${sodium_sources}
)

target_include_directories(
  sodium
  INTERFACE
    "${sodium}/src/libsodium/include"
  PRIVATE
    "${sodium}/src/libsodium/include/sodium"
)

if(target MATCHES "win32")
  add_compile_options(/MT$<$<CONFIG:Debug>:d>)
endif()

if(NOT target MATCHES "win32")
  target_compile_options(
    sodium
    PRIVATE
      -fvisibility=hidden
      -fno-strict-aliasing
      -fwrapv
      -flax-vector-conversions
  )
endif()

target_compile_definitions(
  sodium
  PUBLIC
    SODIUM_STATIC=1
  PRIVATE
    _GNU_SOURCE=1
    CONFIGURED=1
    DEV_MODE=1
    HAVE_ATOMIC_OPS=1
    HAVE_C11_MEMORY_FENCES=1
    HAVE_CET_H=1
    HAVE_GCC_MEMORY_FENCES=1
    HAVE_INLINE_ASM=1
    HAVE_INTTYPES_H=1
    HAVE_STDINT_H=1
    HAVE_TI_MODE=1
)

if(target MATCHES "darwin|ios")
  target_compile_definitions(sodium PRIVATE
    ASM_HIDE_SYMBOL=.private_extern
    TLS=_Thread_local
    HAVE_ARC4RANDOM=1
    HAVE_ARC4RANDOM_BUF=1
    HAVE_CATCHABLE_ABRT=1
    HAVE_CATCHABLE_SEGV=1
    HAVE_CLOCK_GETTIME=1
    HAVE_GETPID=1
    HAVE_MADVISE=1
    HAVE_MEMSET_S=1
    HAVE_MLOCK=1
    HAVE_MMAP=1
    HAVE_MPROTECT=1
    HAVE_NANOSLEEP=1
    HAVE_POSIX_MEMALIGN=1
    HAVE_PTHREAD=1
    HAVE_PTHREAD_PRIO_INHERIT=1
    HAVE_RAISE=1
    HAVE_SYSCONF=1
    HAVE_SYS_MMAN_H=1
    HAVE_SYS_PARAM_H=1
    HAVE_WEAK_SYMBOLS=1
  )
  if(NOT target MATCHES "ios")
    target_compile_definitions(sodium PRIVATE
      HAVE_GETENTROPY=1
      HAVE_SYS_RANDOM_H=1
    )
  endif()
endif()

if(target MATCHES "linux")
  target_compile_definitions(sodium PRIVATE
    ASM_HIDE_SYMBOL=.hidden
    TLS=_Thread_local
    HAVE_CATCHABLE_ABRT=1
    HAVE_CATCHABLE_SEGV=1
    HAVE_CLOCK_GETTIME=1
    HAVE_GETPID=1
    HAVE_MADVISE=1
    HAVE_MLOCK=1
    HAVE_MMAP=1
    HAVE_MPROTECT=1
    HAVE_NANOSLEEP=1
    HAVE_POSIX_MEMALIGN=1
    HAVE_PTHREAD_PRIO_INHERIT=1
    HAVE_PTHREAD=1
    HAVE_RAISE=1
    HAVE_SYSCONF=1
    HAVE_SYS_AUXV_H=1
    HAVE_SYS_MMAN_H=1
    HAVE_SYS_PARAM_H=1
    HAVE_SYS_RANDOM_H=1
    HAVE_WEAK_SYMBOLS=1
  )
endif()

if(target MATCHES "win32")
  target_compile_definitions(sodium PRIVATE
    _CRT_SECURE_NO_WARNINGS=1
    HAVE_RAISE=1
  )
endif()

if(target MATCHES "x64")
  target_compile_definitions(sodium PRIVATE
    HAVE_CPUID=1
    HAVE_RDRAND=1
    HAVE_EMMINTRIN_H=1
    HAVE_PMMINTRIN_H=1
    HAVE_TMMINTRIN_H=1
    HAVE_SMMINTRIN_H=1
    HAVE_WMMINTRIN_H=1
    HAVE_AVXINTRIN_H=1
    HAVE_AVX2INTRIN_H=1
    HAVE_AVX512FINTRIN_H=1
  )
  if(NOT target MATCHES "win32")
    target_compile_definitions(sodium PRIVATE
      HAVE_AMD64_ASM=1
      HAVE_AVX_ASM=1
    )
    target_sources(sodium PRIVATE ${sodium_asm_sources})
  endif()
endif()

if(target MATCHES "arm64")
  target_compile_definitions(sodium PRIVATE HAVE_ARMCRYPTO=1)
endif()

if(CMAKE_C_BYTE_ORDER MATCHES "BIG_ENDIAN")
  target_compile_definitions(sodium PRIVATE NATIVE_BIG_ENDIAN=1)
else()
  target_compile_definitions(sodium PRIVATE NATIVE_LITTLE_ENDIAN=1)
endif()

if(target MATCHES "linux|android")
  target_link_options(sodium PUBLIC -Wl,-z,noexecstack)
endif()

add_library(hc OBJECT)

set_target_properties(
  hc
  PROPERTIES
  C_STANDARD 99
  POSITION_INDEPENDENT_CODE ON
)

target_sources(
  hc
  INTERFACE
    include/hc.h
    include/hc/buffer.h
    include/hc/array.h
    include/hc/core.h
    include/hc/crypto.h
    include/hc/db.h
    include/hc/head.h
    include/hc/keys.h
    include/hc/manifest.h
    include/hc/merkle_tree.h
    include/hc/schema.h
  PRIVATE
    src/hashes.c
    src/core.c
    src/store.c
    src/crypto.c
    src/keys.c
    src/merkle_tree.c
    src/encodings/keys.c
    src/encodings/schema.c
)

target_include_directories(
  hc
  PUBLIC
    include
)

target_link_libraries(
  hc
  PUBLIC
    sodium
    rocksdb
    lexkey
    flattree
)

add_library(hc_shared SHARED)

set_target_properties(
  hc_shared
  PROPERTIES
  OUTPUT_NAME hc
  WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_link_libraries(
  hc_shared
  PUBLIC
    hc
  PRIVATE
    $<TARGET_OBJECTS:sodium>
    rocksdb_shared
    $<TARGET_OBJECTS:compact>
    $<TARGET_OBJECTS:utf>
)

add_library(hc_static STATIC)

set_target_properties(
  hc_static
  PROPERTIES
  OUTPUT_NAME hc
  PREFIX lib
)

target_link_libraries(
  hc_static
  PUBLIC
    hc
  PRIVATE
    $<TARGET_OBJECTS:sodium>
    rocksdb_static
    $<TARGET_OBJECTS:compact>
    $<TARGET_OBJECTS:utf>
)

# rocksdb_static carries C++ object code; force the C++ linker so libc++
# (and its personality routine) are pulled in.
set_target_properties(hc_static PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(hc_shared PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS hc_shared hc_static)

install(FILES include/hc.h DESTINATION include)
install(DIRECTORY include/hc DESTINATION include)

if(PROJECT_IS_TOP_LEVEL)
  enable_testing()

  add_subdirectory(bench)
  add_subdirectory(test)
endif()
