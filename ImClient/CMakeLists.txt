cmake_minimum_required(VERSION 3.19)
project(ImClient LANGUAGES CXX)

# 1. 寻找 Qt 组件：增加了 Network (HTTP用) 和 WebSockets (聊天用)
find_package(Qt6 REQUIRED COMPONENTS Widgets Network WebSockets)
qt_standard_project_setup()

qt_add_executable(ImClient
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

# 2. 链接库文件：把 Network 和 WebSockets 链接进可执行文件
target_link_libraries(ImClient
    PRIVATE
        Qt6::Widgets
        Qt6::Network
        Qt6::WebSockets
)

include(GNUInstallDirs)

install(TARGETS ImClient
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET ImClient
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# Windows 下隐藏控制台窗口 (可选，调试期可以注释掉这行以便看 cout 日志)
set_target_properties(ImClient PROPERTIES
    WIN32_EXECUTABLE ON
)
