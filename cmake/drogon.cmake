include(cmake/jsoncpp.cmake)

# Force static libraries for drogon and trantor so the final binary is
# self-contained inside Docker — no libdrogon.so.1 needs to be present at runtime.
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "Build static libraries" FORCE)

set(BUILD_POSTGRESQL    ON  CACHE BOOL "Build with postgresql support")
set(BUILD_REDIS         ON  CACHE BOOL "Build with redis support")
set(BUILD_DOCUMENTATION OFF CACHE BOOL "Disable drogon docs" FORCE)

add_subdirectory(third_party/drogon)
target_link_libraries(${PROJECT_NAME} PRIVATE drogon)