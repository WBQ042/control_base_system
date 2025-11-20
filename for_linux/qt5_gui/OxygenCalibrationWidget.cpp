/*
 * ===================================================================
 * OxygenCalibrationWidget.cpp
 * Oxygen sensor calibration widget implementation
 * ===================================================================
 */

#include "OxygenCalibrationWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>

using namespace ArduinoHAL;

OxygenCalibrationWidget::OxygenCalibrationWidget(QWidget *parent)
    : QWidget(parent)
    , ui(nullptr)
    , mux(nullptr)
    , oxygenSensor(nullptr)
    , calibrationStorage(nullptr)
    , isCalibrating(false)
    , calibrationStep(0)
    , readingTimer(new QTimer(this))
    , calibrationTimer(new QTimer(this))
{
    setupUI();
    setupConnections();
    initializeSensor();
}

OxygenCalibrationWidget::~OxygenCalibrationWidget()
{
    delete oxygenSensor;
    delete calibrationStorage;
    delete mux;
    delete ui;
}

void OxygenCalibrationWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Calibration buttons
    QGroupBox *calibrationGroup = new QGroupBox("Calibration Controls", this);
    QHBoxLayout *calibrationLayout = new QHBoxLayout(calibrationGroup);
    
    calibrateZeroButton = new QPushButton("Zero Point\n(Short Circuit)", this);
    calibrateAirButton = new QPushButton("Air Point\n(20.95% O2)", this);
    testVoltageButton = new QPushButton("Test Voltage", this);
    clearCalibrationButton = new QPushButton("Clear Calibration", this);
    
    calibrateZeroButton->setStyleSheet("QPushButton { font-size: 12px; padding: 15px; }");
    calibrateAirButton->setStyleSheet("QPushButton { font-size: 12px; padding: 15px; }");
    testVoltageButton->setStyleSheet("QPushButton { font-size: 12px; padding: 10px; }");
    clearCalibrationButton->setStyleSheet("QPushButton { background-color: darkred; font-size: 12px; padding: 10px; }");
    
    calibrationLayout->addWidget(calibrateZeroButton);
    calibrationLayout->addWidget(calibrateAirButton);
    calibrationLayout->addWidget(testVoltageButton);
    calibrationLayout->addWidget(clearCalibrationButton);
    
    mainLayout->addWidget(calibrationGroup);
    
    // Current readings
    QGroupBox *readingsGroup = new QGroupBox("Current Readings", this);
    QGridLayout *readingsLayout = new QGridLayout(readingsGroup);
    
    QLabel *voltageLabel = new QLabel("Voltage (mV):", this);
    voltageLCD = new QLCDNumber(this);
    voltageLCD->setDigitCount(7);
    voltageLCD->setSegmentStyle(QLCDNumber::Flat);
    
    QLabel *oxygenLabel = new QLabel("Oxygen (%):", this);
    oxygenLCD = new QLCDNumber(this);
    oxygenLCD->setDigitCount(5);
    oxygenLCD->setSegmentStyle(QLCDNumber::Flat);
    
    readingsLayout->addWidget(voltageLabel, 0, 0);
    readingsLayout->addWidget(voltageLCD, 0, 1);
    readingsLayout->addWidget(oxygenLabel, 1, 0);
    readingsLayout->addWidget(oxygenLCD, 1, 1);
    
    mainLayout->addWidget(readingsGroup);
    
    // Calibration info
    QGroupBox *infoGroup = new QGroupBox("Calibration Parameters", this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    calibrationStatusLabel = new QLabel("Status: Not Calibrated", this);
    calibrationStatusLabel->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
    
    zeroVoltageLabel = new QLabel("Zero Voltage: N/A", this);
    airVoltageLabel = new QLabel("Air Voltage: N/A", this);
    
    calibrationProgress = new QProgressBar(this);
    calibrationProgress->setVisible(false);
    
    infoLayout->addWidget(calibrationStatusLabel);
    infoLayout->addWidget(zeroVoltageLabel);
    infoLayout->addWidget(airVoltageLabel);
    infoLayout->addWidget(calibrationProgress);
    
    mainLayout->addWidget(infoGroup);
    
    // Instructions
    QGroupBox *instructionsGroup = new QGroupBox("Calibration Instructions", this);
    QVBoxLayout *instructionsLayout = new QVBoxLayout(instructionsGroup);
    
    QLabel *instructions = new QLabel(
        "<b>Zero Point Calibration:</b><br>"
        "1. Short circuit Vsensor+ and Vsensor- pins<br>"
        "2. Click 'Zero Point' button<br>"
        "3. Wait for calibration to complete<br><br>"
        "<b>Air Point Calibration:</b><br>"
        "1. Remove short circuit<br>"
        "2. Expose sensor to fresh air<br>"
        "3. Wait 1-2 minutes for stabilization<br>"
        "4. Click 'Air Point' button<br>"
        "5. Wait for calibration to complete",
        this
    );
    instructions->setWordWrap(true);
    instructionsLayout->addWidget(instructions);
    
    mainLayout->addWidget(instructionsGroup);
}

void OxygenCalibrationWidget::setupConnections()
{
    connect(calibrateZeroButton, &QPushButton::clicked, 
            this, &OxygenCalibrationWidget::onCalibrateZeroClicked);
    connect(calibrateAirButton, &QPushButton::clicked, 
            this, &OxygenCalibrationWidget::onCalibrateAirClicked);
    connect(testVoltageButton, &QPushButton::clicked, 
            this, &OxygenCalibrationWidget::onTestVoltageClicked);
    connect(clearCalibrationButton, &QPushButton::clicked, 
            this, &OxygenCalibrationWidget::onClearCalibrationClicked);
    
    connect(readingTimer, &QTimer::timeout, 
            this, &OxygenCalibrationWidget::updateReading);
    connect(calibrationTimer, &QTimer::timeout, 
            this, &OxygenCalibrationWidget::performZeroCalibration);
}

void OxygenCalibrationWidget::initializeSensor()
{
    Wire.begin();
    
    // Initialize I2C Mux
    mux = new I2CMux(0x70);
    mux->begin();
    mux->addChannel(6, 0x4A, "ADS1115");
    
    // Initialize oxygen sensor
    oxygenSensor = new AO08_Sensor(mux, 6, 0x4A);
    calibrationStorage = new AO08_CalibrationStorage();
    
    if (!calibrationStorage->begin()) {
        QMessageBox::warning(this, "Warning",
            "Calibration storage initialization failed.\n"
            "Calibration data will not be persistent.");
    }
    
    bool success = oxygenSensor->begin();
    emit i2cStatusChanged(success);
    
    if (!success) {
        QMessageBox::critical(this, "Initialization Error",
            "Failed to initialize oxygen sensor!\n\n"
            "Please check:\n"
            "1. ADS1115 connection (I2C address 0x4A)\n"
            "2. I2C Mux channel 6 configuration\n"
            "3. Device permissions");
        
        calibrateZeroButton->setEnabled(false);
        calibrateAirButton->setEnabled(false);
        return;
    }
    
    updateCalibrationInfo();
    
    // Start reading timer (update every 500ms)
    readingTimer->start(500);
}

void OxygenCalibrationWidget::updateCalibrationInfo()
{
    if (oxygenSensor->isCalibrated()) {
        float vZero, vAir;
        oxygenSensor->getCalibrationParams(vZero, vAir);
        
        calibrationStatusLabel->setText("Status: Calibrated");
        calibrationStatusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
        
        zeroVoltageLabel->setText(QString("Zero Voltage: %1 mV").arg(vZero, 0, 'f', 4));
        airVoltageLabel->setText(QString("Air Voltage: %1 mV").arg(vAir, 0, 'f', 4));
    } else {
        calibrationStatusLabel->setText("Status: Not Calibrated");
        calibrationStatusLabel->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
        
        zeroVoltageLabel->setText("Zero Voltage: N/A");
        airVoltageLabel->setText("Air Voltage: N/A");
    }
}

void OxygenCalibrationWidget::onCalibrateZeroClicked()
{
    int ret = QMessageBox::question(this, "Zero Point Calibration",
        "Have you short-circuited Vsensor+ and Vsensor- pins?\n\n"
        "Click 'Yes' to proceed with calibration.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        isCalibrating = true;
        calibrateZeroButton->setEnabled(false);
        calibrateAirButton->setEnabled(false);
        calibrationProgress->setVisible(true);
        calibrationProgress->setRange(0, 100);
        calibrationProgress->setValue(0);
        
        // Start calibration after 2 second delay
        QTimer::singleShot(2000, this, &OxygenCalibrationWidget::performZeroCalibration);
    }
}

void OxygenCalibrationWidget::performZeroCalibration()
{
    bool success = oxygenSensor->calibrateZero(true);
    
    isCalibrating = false;
    calibrateZeroButton->setEnabled(true);
    calibrateAirButton->setEnabled(true);
    calibrationProgress->setVisible(false);
    
    if (success) {
        QMessageBox::information(this, "Success",
            "Zero point calibration completed successfully!");
        updateCalibrationInfo();
    } else {
        QMessageBox::critical(this, "Error",
            "Zero point calibration failed!\n\n"
            "Please check sensor connection and try again.");
    }
}

void OxygenCalibrationWidget::onCalibrateAirClicked()
{
    int ret = QMessageBox::question(this, "Air Point Calibration",
        "Is the sensor exposed to fresh air?\n"
        "Has it stabilized for at least 1-2 minutes?\n\n"
        "Click 'Yes' to proceed with calibration.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        isCalibrating = true;
        calibrateZeroButton->setEnabled(false);
        calibrateAirButton->setEnabled(false);
        calibrationProgress->setVisible(true);
        calibrationProgress->setRange(0, 100);
        calibrationProgress->setValue(0);
        
        QTimer::singleShot(2000, this, &OxygenCalibrationWidget::performAirCalibration);
    }
}

void OxygenCalibrationWidget::performAirCalibration()
{
    bool success = oxygenSensor->calibrateAir(true);
    
    isCalibrating = false;
    calibrateZeroButton->setEnabled(true);
    calibrateAirButton->setEnabled(true);
    calibrationProgress->setVisible(false);
    
    if (success) {
        QMessageBox::information(this, "Success",
            "Air point calibration completed successfully!\n\n"
            "Calibration data has been saved.");
        updateCalibrationInfo();
    } else {
        QMessageBox::critical(this, "Error",
            "Air point calibration failed!\n\n"
            "Please ensure zero point calibration was completed first.");
    }
}

void OxygenCalibrationWidget::onTestVoltageClicked()
{
    float voltage;
    if (oxygenSensor->readVoltage(voltage)) {
        QMessageBox::information(this, "Voltage Test",
            QString("Current sensor voltage: %1 mV\n\n"
                    "Sensor connection: OK").arg(voltage, 0, 'f', 4));
    } else {
        QMessageBox::critical(this, "Voltage Test",
            "Failed to read sensor voltage!\n\n"
            "Please check I2C connection.");
    }
}

void OxygenCalibrationWidget::onClearCalibrationClicked()
{
    int ret = QMessageBox::warning(this, "Clear Calibration",
        "Are you sure you want to clear all calibration data?\n\n"
        "You will need to recalibrate the sensor.",
        QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        if (calibrationStorage->clearCalibration()) {
            QMessageBox::information(this, "Success",
                "Calibration data cleared.\n\n"
                "Please recalibrate the sensor.");
            updateCalibrationInfo();
        }
    }
}

void OxygenCalibrationWidget::updateReading()
{
    if (isCalibrating) {
        return;
    }
    
    // Read voltage
    float voltage;
    if (oxygenSensor->readVoltage(voltage)) {
        voltageLCD->display(voltage);
    } else {
        voltageLCD->display(0.0);
    }
    
    // Read oxygen percentage
    float oxygenPercent;
    if (oxygenSensor->readOxygenPercentage(oxygenPercent)) {
        oxygenLCD->display(oxygenPercent);
    } else {
        oxygenLCD->display(0.0);
    }
}
