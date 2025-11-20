/*
 * ===================================================================
 * SensorDataPlot.cpp
 * Real-time sensor data plotting implementation
 * ===================================================================
 */

#include "SensorDataPlot.h"
#include <QVBoxLayout>

SensorDataPlot::SensorDataPlot(const QString &title, QWidget *parent)
    : QWidget(parent)
    , maxDataPoints(100)
    , currentIndex(0)
{
    setupChart(title);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(chartView);
    layout->setContentsMargins(0, 0, 0, 0);
}

SensorDataPlot::~SensorDataPlot()
{
}

void SensorDataPlot::setupChart(const QString &title)
{
    // Create chart
    chart = new QChart();
    chart->setTitle(title);
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->hide();
    
    // Create series
    series = new QLineSeries();
    QPen pen(Qt::cyan);
    pen.setWidth(2);
    series->setPen(pen);
    
    chart->addSeries(series);
    
    // Create axes
    axisX = new QValueAxis();
    axisX->setTitleText("Time (samples)");
    axisX->setRange(0, maxDataPoints);
    axisX->setTickCount(11);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    
    axisY = new QValueAxis();
    axisY->setTitleText("Value");
    axisY->setRange(-10, 100);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    
    // Create chart view
    chartView = new QChartView(chart, this);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    // Initialize data buffer
    dataBuffer.reserve(maxDataPoints);
}

void SensorDataPlot::addDataPoint(double value)
{
    dataBuffer.append(value);
    
    // Keep buffer size limited
    if (dataBuffer.size() > maxDataPoints) {
        dataBuffer.removeFirst();
    }
    
    currentIndex++;
    updateChart();
}

void SensorDataPlot::clearData()
{
    dataBuffer.clear();
    currentIndex = 0;
    series->clear();
}

void SensorDataPlot::setYAxisRange(double min, double max)
{
    axisY->setRange(min, max);
}

void SensorDataPlot::updateChart()
{
    series->clear();
    
    int startIndex = qMax(0, currentIndex - maxDataPoints);
    
    for (int i = 0; i < dataBuffer.size(); ++i) {
        series->append(startIndex + i, dataBuffer[i]);
    }
    
    // Auto-scale Y axis based on current data
    if (!dataBuffer.isEmpty()) {
        double minVal = *std::min_element(dataBuffer.begin(), dataBuffer.end());
        double maxVal = *std::max_element(dataBuffer.begin(), dataBuffer.end());
        
        double margin = (maxVal - minVal) * 0.1;
        axisY->setRange(minVal - margin, maxVal + margin);
    }
    
    // Update X axis range to show latest data
    if (currentIndex > maxDataPoints) {
        axisX->setRange(currentIndex - maxDataPoints, currentIndex);
    }
}
