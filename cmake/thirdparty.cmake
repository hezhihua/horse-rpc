
add_custom_target(thirdparty)
include(ExternalProject)
 
set(SPDLOG_ROOT          ${CMAKE_BINARY_DIR}/thirdparty/spdlog)
set(SPDLOG_LIB_DIR       ${SPDLOG_ROOT}/lib)
set(SPDLOG_INCLUDE_DIR   ${SPDLOG_ROOT}/include)
 
#set(SPDLOG_URL           https://github.com/gabime/spdlog/archive/v1.8.1.tar.gz)
set(SPDLOG_URL           https://github.com.cnpmjs.org/gabime/spdlog/archive/v1.8.1.tar.gz)
set(SPDLOG_CONFIGURE     cd ${SPDLOG_ROOT}/src/spdlog-1.8.1 && cmake -D CMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/third-party/spdlog .)
set(SPDLOG_MAKE          cd ${SPDLOG_ROOT}/src/spdlog-1.8.1 && make)
set(SPDLOG_INSTALL       cd ${SPDLOG_ROOT}/src/spdlog-1.8.1 && make install)

ExternalProject_Add(spdlog-1.8.1
        URL                   ${SPDLOG_URL}
        DOWNLOAD_NAME         spdlog-1.8.1.tar.gz
        PREFIX                ${SPDLOG_ROOT}
        CONFIGURE_COMMAND     ${SPDLOG_CONFIGURE}
        BUILD_COMMAND         ${SPDLOG_MAKE}
        INSTALL_COMMAND       ${SPDLOG_INSTALL}
        BUILD_ALWAYS          0
)


add_dependencies(thirdparty spdlog-1.8.1)
