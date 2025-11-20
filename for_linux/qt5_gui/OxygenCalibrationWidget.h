/*
 * ===================================================================
 * OxygenCalibrationWidget.h
 * Oxygen sensor calibration interface
 * ===================================================================
 */

#ifndef OXYGENCALIBRATIONWIDGET_H
#define OXYGENCALIBRATIONWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLCDNumber>
#include <QTimer>
#include <QProgressBar>
#include "/home/wang/code/AO08/AO08_Sensor.h"
#include "/home/wang/code/AO08/AO08_CalibrationStorage.h"
#include "/home/wang/code/breath_contr/I2CMux.h"

namespace Ui {
class OxygenCalibrationWidget;
}

class OxygenCalibrationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OxygenCalibrationWidget(QWidget *parent = nullptr);
    ~OxygenCalibrationWidget();

signals:
    void i2cStatusChanged(bool connected);

private slots:
    void onCalibrateZeroClicked();
    void onCalibrateAirClicked();
    void onTestVoltageClicked();
    void onClearCalibrationClicked();
    void updateReading();
    void performZeroCalibration();
    void performAirCalibration();

private:
    Ui::OxygenCalibrationWidget *ui;
    
    // Backend objects
    I2CMux *mux;
    AO08_Sensor *oxygenSensor;
    AO08_CalibrationStorage *calibrationStorage;
    
    // UI elements
    QPushButton *calibrateZeroButton;
    QPushButton *calibrateAirButton;
    QPushButton *testVoltageButton;
    QPushButton *clearCalibrationButton;
    
    QLCDNumber *voltageLCD;
    QLCDNumber *oxygenLCD;
    
    QLabel *calibrationStatusLabel;
    QLabel *zeroVoltageLabel;
    QLabel *airVoltageLabel;
    QProgressBar *calibrationProgress;
    
    QTimer *readingTimer;
    QTimer *calibrationTimer;
    
    bool isCalibrating;
    int calibrationStep;
    
    void setupUI();
    void setupConnections();
    void initializeSensor();
    void updateCalibrationInfo();
};

#endif // OXYGENCALIBRATIONWIDGET_H
