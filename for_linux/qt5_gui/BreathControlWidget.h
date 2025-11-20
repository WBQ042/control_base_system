/*
 * ===================================================================
 * BreathControlWidget.h
 * Breath controller monitoring and control interface
 * ===================================================================
 */

#ifndef BREATHCONTROLWIDGET_H
#define BREATHCONTROLWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QLCDNumber>
#include "/home/wang/code/breath_contr/LuckfoxArduino.h"
#include "/home/wang/code/breath_contr/BreathController.h"
#include "SensorDataPlot.h"

namespace Ui {
class BreathControlWidget;
}

class BreathControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BreathControlWidget(QWidget *parent = nullptr);
    ~BreathControlWidget();

signals:
    void i2cStatusChanged(bool connected);

private slots:
    void onStartClicked();
    void onStopClicked();
    void onResetClicked();
    void updateSensorData();
    void onSensorInitialized(bool success);

private:
    Ui::BreathControlWidget *ui;
    
    // Backend controller
    BreathController *controller;
    bool isRunning;
    
    // UI elements
    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *resetButton;
    
    // LCD displays
    QLCDNumber *pressureLCD;
    QLCDNumber *flowLCD;
    QLCDNumber *co2LCD;
    QLCDNumber *o2LCD;
    
    // Charts
    SensorDataPlot *pressurePlot;
    SensorDataPlot *flowPlot;
    
    // Timer for data updates
    QTimer *updateTimer;
    
    void setupUI();
    void setupConnections();
    void initializeController();
    void updateDisplays();
};

#endif // BREATHCONTROLWIDGET_H
