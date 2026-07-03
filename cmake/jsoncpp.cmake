include(FetchContent)

# Must be set BEFORE FetchContent_MakeAvailable so jsoncpp builds as a static
# archive (.a) rather than a shared object (.so). Without this, jsoncpp.cmake
# runs before drogon.cmake has a chance to set BUILD_SHARED_LIBS=OFF, which
# causes libjsoncpp.so.26 to be produced — a file that does not exist in the
# lean runtime Docker image.
set(BUILD_SHARED_LIBS                OFF CACHE BOOL "Build static libraries" FORCE)
set(JSONCPP_WITH_TESTS               OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG        1.9.6
)

FetchContent_MakeAvailable(jsoncpp)
set(JSONCPP_INCLUDE_DIRS "${jsoncpp_SOURCE_DIR}/include" CACHE INTERNAL "")

# Explicitly reference the static target (jsoncpp_static) rather than the
# alias jsoncpp_lib, which respects BUILD_SHARED_LIBS and can resolve to
# the shared variant on some cmake versions.
set(JSONCPP_LIBRARIES jsoncpp_static CACHE INTERNAL "")