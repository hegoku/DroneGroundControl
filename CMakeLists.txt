cmake_minimum_required(VERSION 3.16)

project(DroneGroundControl VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick SerialPort Network Graphs)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(appDroneGroundControl
    main.cpp resource.qrc
    src/app/ConnectionSession.h
    src/app/ConnectionSession.cpp
    src/app/AnotcRequestManager.h
    src/app/AnotcRequestManager.cpp
    src/app/ParameterStore.h
    src/app/ParameterStore.cpp
    src/app/Flight.h
    src/app/Flight.cpp
    src/app/DataChartModel.h
    src/app/DataChartModel.cpp
    src/app/DataChartGraphsAdapter.h
    src/app/DataChartGraphsAdapter.cpp
    src/app/DataChartBenchmarkRunner.h
    src/app/DataChartBenchmarkRunner.cpp
    src/app/DataChartView.h
    src/app/DataChartView.cpp
    src/app/DataFrameTableModel.h
    src/app/DataFrameTableModel.cpp
    src/connection/ConnectionConfig.h
    src/connection/ITransport.h
    src/connection/SerialTransport.h
    src/connection/SerialTransport.cpp
    src/connection/UdpTransport.h
    src/connection/UdpTransport.cpp
    src/protocol/AnotcFrame.h
    src/protocol/ProtocolTypes.h
    src/protocol/IProtocolSession.h
    src/protocol/AnotcResponseMatcher.h
    src/protocol/AnotcResponseMatcher.cpp
    src/protocol/AnotcProtocolSession.h
    src/protocol/AnotcProtocolSession.cpp
)

target_include_directories(appDroneGroundControl
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)

qt_add_qml_module(appDroneGroundControl
    URI DroneGroundControl
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/pages/HomePage.qml
        qml/pages/ParameterPage.qml
        qml/pages/DataChartPage.qml
        qml/pages/DataChartGraphsPage.qml
        qml/pages/DataFrameTablePage.qml
        qml/components/Battery.qml
        qml/components/SensorStatus.qml
        qml/components/ConnectionButton.qml
        qml/components/SerialPortConnection.qml
        qml/components/UdpConnection.qml
    RESOURCES resource.qrc

)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appDroneGroundControl PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appDroneGroundControl
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appDroneGroundControl
    PRIVATE Qt6::Quick Qt6::SerialPort Qt6::Network Qt6::Graphs
)

include(GNUInstallDirs)
install(TARGETS appDroneGroundControl
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
