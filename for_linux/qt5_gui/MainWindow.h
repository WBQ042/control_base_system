/*
 * ===================================================================
 * MainWindow.h
 * Main application window with tabbed interface
 * ===================================================================
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QStatusBar>
#include <QTimer>
#include <QLabel>
#include "BreathControlWidget.h"
#include "OxygenCalibrationWidget.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTabChanged(int index);
    void updateStatusBar();
    void onI2CStatusChanged(bool connected);

private:
    Ui::MainWindow *ui;
    
    // Tab widgets
    QTabWidget *tabWidget;
    BreathControlWidget *breathControlWidget;
    OxygenCalibrationWidget *oxygenCalibrationWidget;
    
    // Status bar elements
    QLabel *statusLabel;
    QLabel *i2cStatusLabel;
    QLabel *timeLabel;
    QTimer *statusTimer;
    
    void setupUI();
    void setupStatusBar();
    void setupConnections();
};

#endif // MAINWINDOW_H
