cmake_minimum_required(VERSION 3.16)

project(ComStudio VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    SerialPort
    PrintSupport  # Required by QCustomPlot
)

# Collect source files
set(CORE_SOURCES
    src/core/SerialManager.cpp
    src/core/ProtocolHandler.cpp
    src/core/LineParser.cpp
)

set(CORE_HEADERS
    include/core/SerialManager.h
    include/core/BaseProtocol.h
    include/core/GenericDataPacket.h
    include/core/ProtocolHandler.h
    include/core/LineParser.h
    include/core/ParserConfig.h
)

set(MODEL_SOURCES
    src/models/DataBuffer.cpp
)

set(MODEL_HEADERS
    include/models/DataBuffer.h
)

set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/SerialSettingsWidget.cpp
    src/ui/TerminalWidget.cpp
    src/ui/PlotterWidget.cpp
    src/ui/ParserConfigWidget.cpp
    src/ui/AutoSendDialog.cpp
    src/ui/RecordingWidget.cpp
    src/ui/ChannelPlotWindow.cpp
)

set(UI_HEADERS
    include/ui/MainWindow.h
    include/ui/SerialSettingsWidget.h
    include/ui/TerminalWidget.h
    include/ui/PlotterWidget.h
    include/ui/ParserConfigWidget.h
    include/ui/AutoSendDialog.h
    include/ui/RecordingWidget.h
    include/ui/ChannelPlotWindow.h
)

set(UI_FORMS
    src/ui/MainWindow.ui
    src/ui/SerialSettingsWidget.ui
)

# QCustomPlot library
set(QCUSTOMPLOT_SOURCES
    third_party/QCustomPlot/qcustomplot.cpp
)
set(QCUSTOMPLOT_HEADERS
    third_party/QCustomPlot/qcustomplot.h
)

# Resources
set(RESOURCES
    assets/resources.qrc
)

# Create executable
add_executable(ComStudio
    src/main.cpp
    ${CORE_SOURCES}
    ${CORE_HEADERS}
    ${MODEL_SOURCES}
    ${MODEL_HEADERS}
    ${UI_SOURCES}
    ${UI_HEADERS}
    ${UI_FORMS}
    ${QCUSTOMPLOT_SOURCES}
    ${QCUSTOMPLOT_HEADERS}
    ${RESOURCES}
)

# Include directories
target_include_directories(ComStudio PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/QCustomPlot
)

# Link Qt libraries
target_link_libraries(ComStudio PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::SerialPort
    Qt6::PrintSupport
)

# Windows-specific settings
set_target_properties(ComStudio PROPERTIES
    WIN32_EXECUTABLE TRUE
)

# Windows icon
if (WIN32)
    set(APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons/serial_terminal.ico")
    if (EXISTS ${APP_ICON})
        target_sources(ComStudio PRIVATE app.rc)
    endif()
endif()

# Install rules
include(GNUInstallDirs)
install(TARGETS ComStudio
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
