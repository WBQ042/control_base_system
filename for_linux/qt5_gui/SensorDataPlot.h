/*
 * ===================================================================
 * SensorDataPlot.h
 * Real-time sensor data plotting widget
 * ===================================================================
 */

#ifndef SENSORDATAPLOT_H
#define SENSORDATAPLOT_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVector>

QT_CHARTS_USE_NAMESPACE

class SensorDataPlot : public QWidget
{
    Q_OBJECT

public:
    explicit SensorDataPlot(const QString &title, QWidget *parent = nullptr);
    ~SensorDataPlot();
    
    void addDataPoint(double value);
    void clearData();
    void setYAxisRange(double min, double max);

private:
    QChartView *chartView;
    QChart *chart;
    QLineSeries *series;
    QValueAxis *axisX;
    QValueAxis *axisY;
    
    QVector<double> dataBuffer;
    int maxDataPoints;
    int currentIndex;
    
    void setupChart(const QString &title);
    void updateChart();
};

#endif // SENSORDATAPLOT_H
