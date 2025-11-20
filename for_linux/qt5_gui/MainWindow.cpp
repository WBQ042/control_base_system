/*
 * ===================================================================
 * MainWindow.cpp
 * Main application window implementation
 * ===================================================================
 */

#include "MainWindow.h"
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(nullptr)
    , statusTimer(new QTimer(this))
{
    setupUI();
    setupStatusBar();
    setupConnections();
    
    // Update status bar every second
    statusTimer->start(1000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    // Set window properties
    setWindowTitle("Ventilator Control & Calibration System");
    resize(800, 600);
    
    // Create central widget with tab widget
    tabWidget = new QTabWidget(this);
    setCentralWidget(tabWidget);
    
    // Create and add tab widgets
    breathControlWidget = new BreathControlWidget(this);
    oxygenCalibrationWidget = new OxygenCalibrationWidget(this);
    
    tabWidget->addTab(breathControlWidget, "Breath Controller");
    tabWidget->addTab(oxygenCalibrationWidget, "O2 Calibration");
    
    // Create menu bar
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("&File");
    
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    fileMenu->addAction(exitAction);
    
    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    
    QAction *aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About",
            "Ventilator Control & Calibration System\n\n"
            "Version 1.0\n"
            "Hardware: Luckfox Embedded Linux\n"
            "Framework: Qt5\n\n"
            "© 2024 Medical Devices");
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *status = statusBar();
    
    // Status label
    statusLabel = new QLabel("Ready", this);
    status->addWidget(statusLabel);
    
    // I2C status indicator
    i2cStatusLabel = new QLabel("I2C: Unknown", this);
    i2cStatusLabel->setStyleSheet("QLabel { color: orange; }");
    status->addPermanentWidget(i2cStatusLabel);
    
    // Time label
    timeLabel = new QLabel(this);
    status->addPermanentWidget(timeLabel);
}

void MainWindow::setupConnections()
{
    // Tab change handler
    connect(tabWidget, &QTabWidget::currentChanged, 
            this, &MainWindow::onTabChanged);
    
    // Status bar timer
    connect(statusTimer, &QTimer::timeout, 
            this, &MainWindow::updateStatusBar);
    
    // I2C status updates from widgets
    connect(breathControlWidget, &BreathControlWidget::i2cStatusChanged,
            this, &MainWindow::onI2CStatusChanged);
    connect(oxygenCalibrationWidget, &OxygenCalibrationWidget::i2cStatusChanged,
            this, &MainWindow::onI2CStatusChanged);
}

void MainWindow::onTabChanged(int index)
{
    QString tabName = tabWidget->tabText(index);
    statusLabel->setText("Switched to: " + tabName);
}

void MainWindow::updateStatusBar()
{
    // Update time display
    QDateTime now = QDateTime::currentDateTime();
    timeLabel->setText(now.toString("yyyy-MM-dd hh:mm:ss"));
}

void MainWindow::onI2CStatusChanged(bool connected)
{
    if (connected) {
        i2cStatusLabel->setText("I2C: Connected");
        i2cStatusLabel->setStyleSheet("QLabel { color: green; }");
    } else {
        i2cStatusLabel->setText("I2C: Disconnected");
        i2cStatusLabel->setStyleSheet("QLabel { color: red; }");
    }
}
