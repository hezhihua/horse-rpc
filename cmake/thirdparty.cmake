
add_custom_target(thirdparty)
include(ExternalProject)
 
 #spdlog 库
set(SPDLOG_ROOT          ${CMAKE_BINARY_DIR}/thirdparty/spdlog)
 
#set(SPDLOG_URL           https://github.com/gabime/spdlog.git)
set(SPDLOG_URL           https://github.com.cnpmjs.org/gabime/spdlog.git)
set(SPDLOG_CONFIGURE     cd ${SPDLOG_ROOT}/src/spdlog-1.8.1 && cmake -D CMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/third-party/spdlog .)
set(SPDLOG_MAKE          cd ${SPDLOG_ROOT}/src/spdlog-1.8.1 && make)
set(SPDLOG_INSTALL       cd ${SPDLOG_ROOT}/src/spdlog-1.8.1 && make install)

ExternalProject_Add(spdlog-1.8.1
        GIT_REPOSITORY        ${SPDLOG_URL}
        GIT_TAG               v1.8.1
        PREFIX                ${SPDLOG_ROOT}
        CONFIGURE_COMMAND     ${SPDLOG_CONFIGURE}
        BUILD_COMMAND         ${SPDLOG_MAKE}
        INSTALL_COMMAND       ${SPDLOG_INSTALL}
        BUILD_ALWAYS          0
)
add_dependencies(thirdparty spdlog-1.8.1)