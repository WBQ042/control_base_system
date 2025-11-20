/*
 * ===================================================================
 * BreathControlWidget.cpp
 * Breath controller widget implementation
 * ===================================================================
 */

#include "BreathControlWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

using namespace ArduinoHAL;

BreathControlWidget::BreathControlWidget(QWidget *parent)
    : QWidget(parent)
    , ui(nullptr)
    , controller(nullptr)
    , isRunning(false)
    , updateTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    initializeController();
}

BreathControlWidget::~BreathControlWidget()
{
    if (isRunning) {
        onStopClicked();
    }
    delete controller;
    delete ui;
}

void BreathControlWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    startButton = new QPushButton("Start Monitoring", this);
    stopButton = new QPushButton("Stop", this);
    resetButton = new QPushButton("Reset", this);
    
    startButton->setStyleSheet("QPushButton { background-color: green; font-size: 14px; padding: 10px; }");
    stopButton->setStyleSheet("QPushButton { background-color: red; font-size: 14px; padding: 10px; }");
    stopButton->setEnabled(false);
    
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addWidget(resetButton);
    mainLayout->addLayout(buttonLayout);
    
    // Sensor readings group
    QGroupBox *sensorGroup = new QGroupBox("Sensor Readings", this);
    QGridLayout *sensorLayout = new QGridLayout(sensorGroup);
    
    // Pressure
    QLabel *pressureLabel = new QLabel("Pressure (cmH2O):", this);
    pressureLCD = new QLCDNumber(this);
    pressureLCD->setDigitCount(6);
    pressureLCD->setSegmentStyle(QLCDNumber::Flat);
    sensorLayout->addWidget(pressureLabel, 0, 0);
    sensorLayout->addWidget(pressureLCD, 0, 1);
    
    // Flow
    QLabel *flowLabel = new QLabel("Flow (L/min):", this);
    flowLCD = new QLCDNumber(this);
    flowLCD->setDigitCount(6);
    flowLCD->setSegmentStyle(QLCDNumber::Flat);
    sensorLayout->addWidget(flowLabel, 1, 0);
    sensorLayout->addWidget(flowLCD, 1, 1);
    
    // CO2
    QLabel *co2Label = new QLabel("CO2 (%):", this);
    co2LCD = new QLCDNumber(this);
    co2LCD->setDigitCount(5);
    co2LCD->setSegmentStyle(QLCDNumber::Flat);
    sensorLayout->addWidget(co2Label, 0, 2);
    sensorLayout->addWidget(co2LCD, 0, 3);
    
    // O2
    QLabel *o2Label = new QLabel("O2 (%):", this);
    o2LCD = new QLCDNumber(this);
    o2LCD->setDigitCount(5);
    o2LCD->setSegmentStyle(QLCDNumber::Flat);
    sensorLayout->addWidget(o2Label, 1, 2);
    sensorLayout->addWidget(o2LCD, 1, 3);
    
    mainLayout->addWidget(sensorGroup);
    
    // Charts
    QHBoxLayout *chartLayout = new QHBoxLayout();
    
    pressurePlot = new SensorDataPlot("Pressure (cmH2O)", this);
    flowPlot = new SensorDataPlot("Flow (L/min)", this);
    
    chartLayout->addWidget(pressurePlot);
    chartLayout->addWidget(flowPlot);
    
    mainLayout->addLayout(chartLayout);
}

void BreathControlWidget::setupConnections()
{
    connect(startButton, &QPushButton::clicked, this, &BreathControlWidget::onStartClicked);
    connect(stopButton, &QPushButton::clicked, this, &BreathControlWidget::onStopClicked);
    connect(resetButton, &QPushButton::clicked, this, &BreathControlWidget::onResetClicked);
    connect(updateTimer, &QTimer::timeout, this, &BreathControlWidget::updateSensorData);
}

void BreathControlWidget::initializeController()
{
    // Initialize I2C
    Wire.begin();
    
    // Create breath controller
    controller = new BreathController();
    
    // Initialize (begin() returns void)
    controller->begin();
    
    // Assume success for now - could add error detection later
    onSensorInitialized(true);
}

void BreathControlWidget::onSensorInitialized(bool success)
{
    emit i2cStatusChanged(success);
    
    if (!success) {
        QMessageBox::critical(this, "Initialization Error",
            "Failed to initialize sensors!\n\n"
            "Please check:\n"
            "1. I2C connections\n"
            "2. Device permissions (/dev/i2c-0)\n"
            "3. Hardware power supply");
        startButton->setEnabled(false);
    }
}

void BreathControlWidget::onStartClicked()
{
    isRunning = true;
    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    
    // Start update timer (update every 100ms)
    updateTimer->start(100);
}

void BreathControlWidget::onStopClicked()
{
    isRunning = false;
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    
    updateTimer->stop();
}

void BreathControlWidget::onResetClicked()
{
    pressurePlot->clearData();
    flowPlot->clearData();
    
    pressureLCD->display(0.0);
    flowLCD->display(0.0);
    co2LCD->display(0.0);
    o2LCD->display(0.0);
}

void BreathControlWidget::updateSensorData()
{
    if (!controller || !isRunning) {
        return;
    }
    
    // Update controller (reads all sensors)
    controller->update();
    
    // Get sensor data
    float pressure = controller->getPressure();
    float flow = controller->getFlow();
    float co2 = controller->getCO2Percentage();
    float o2 = controller->getO2Percentage();
    
    // Update LCD displays
    pressureLCD->display(pressure);
    flowLCD->display(flow);
    co2LCD->display(co2);
    o2LCD->display(o2);
    
    // Update charts
    pressurePlot->addDataPoint(pressure);
    flowPlot->addDataPoint(flow);
}
