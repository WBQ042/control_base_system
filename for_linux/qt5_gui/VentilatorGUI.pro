#-------------------------------------------------
# Project: Ventilator Control & Calibration GUI
# Target: Luckfox Embedded Linux with Qt5
#-------------------------------------------------

QT       += core gui widgets charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VentilatorGUI
TEMPLATE = app

# C++11 support
CONFIG += c++11

# Compiler flags
QMAKE_CXXFLAGS += -std=c++11 -Wall

# Architecture-specific settings
unix {
    # Cross-compilation for ARM
    # Uncomment and adjust for cross-compilation
    # QMAKE_CXX = arm-none-linux-gnueabihf-g++
    # QMAKE_LINK = arm-none-linux-gnueabihf-g++
}

# Include paths
INCLUDEPATH += . \
               /home/wang/code/breath_contr \
               /home/wang/code/AO08

# Source files
SOURCES += \
    main.cpp \
    MainWindow.cpp \
    BreathControlWidget.cpp \
    OxygenCalibrationWidget.cpp \
    SensorDataPlot.cpp \
    /home/wang/code/breath_contr/BreathController.cpp \
    /home/wang/code/breath_contr/ADS1115.cpp \
    /home/wang/code/breath_contr/gas_concentration.cpp \
    /home/wang/code/breath_contr/oxygen_sensor.cpp \
    /home/wang/code/breath_contr/I2CMux.cpp \
    /home/wang/code/breath_contr/OLEDDisplay.cpp \
    /home/wang/code/AO08/AO08_Sensor.cpp \
    /home/wang/code/AO08/AO08_CalibrationStorage.cpp

# Header files
HEADERS += \
    MainWindow.h \
    BreathControlWidget.h \
    OxygenCalibrationWidget.h \
    SensorDataPlot.h \
    /home/wang/code/breath_contr/LuckfoxArduino.h \
    /home/wang/code/breath_contr/BreathController.h \
    /home/wang/code/breath_contr/ADS1115.h \
    /home/wang/code/breath_contr/gas_concentration.h \
    /home/wang/code/breath_contr/oxygen_sensor.h \
    /home/wang/code/breath_contr/I2CMux.h \
    /home/wang/code/breath_contr/OLEDDisplay.h \
    /home/wang/code/AO08/AO08_Sensor.h \
    /home/wang/code/AO08/AO08_CalibrationStorage.h

# Resources
RESOURCES += \
    resources.qrc

# Libraries
LIBS += -lpthread -lm

# Installation
target.path = /root
INSTALLS += target

# Build directory
OBJECTS_DIR = build/obj
MOC_DIR = build/moc
UI_DIR = build/ui
RCC_DIR = build/rcc
