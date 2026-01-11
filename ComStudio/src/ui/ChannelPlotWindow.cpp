/**
 * @file ChannelPlotWindow.cpp
 * @brief Implementation of ChannelPlotWindow
 */

#include "ui/ChannelPlotWindow.h"
#include "qcustomplot.h"

#include <QVBoxLayout>
#include <QCloseEvent>

ChannelPlotWindow::ChannelPlotWindow(int channelIndex, const QString &channelName,
                                     const QColor &color, QWidget *parent)
    : QWidget(parent, Qt::Window)
    , m_channelIndex(channelIndex)
    , m_channelName(channelName)
    , m_color(color)
{
    setWindowTitle(tr("Channel: %1").arg(channelName));
    setMinimumSize(400, 300);
    
    setupUi();
    setupPlot();
}

ChannelPlotWindow::~ChannelPlotWindow()
{
}

void ChannelPlotWindow::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_plot = new QCustomPlot();
    layout->addWidget(m_plot);
}

void ChannelPlotWindow::setupPlot()
{
    // Configure dark theme (matching main plotter)
    m_plot->setBackground(QBrush(QColor(0x1e, 0x1e, 0x2e)));
    
    // X Axis
    m_plot->xAxis->setBasePen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->xAxis->setTickPen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->xAxis->setSubTickPen(QPen(QColor(0x31, 0x32, 0x44)));
    m_plot->xAxis->setTickLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->xAxis->setLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->xAxis->setLabel("Time (s)");
    m_plot->xAxis->grid()->setPen(QPen(QColor(0x31, 0x32, 0x44), 1, Qt::DotLine));
    
    // Y Axis
    m_plot->yAxis->setBasePen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->yAxis->setTickPen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->yAxis->setSubTickPen(QPen(QColor(0x31, 0x32, 0x44)));
    m_plot->yAxis->setTickLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->yAxis->setLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->yAxis->setLabel(m_channelName);
    m_plot->yAxis->grid()->setPen(QPen(QColor(0x31, 0x32, 0x44), 1, Qt::DotLine));
    
    // Interactions
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    m_plot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    m_plot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    
    // Create graph
    m_graph = m_plot->addGraph();
    m_graph->setName(m_channelName);
    m_graph->setPen(QPen(m_color, 2));
    m_graph->setAntialiased(true);
}

void ChannelPlotWindow::updateData(const QVector<double> &timestamps, const QVector<double> &values)
{
    if (!m_graph) return;
    
    m_graph->setData(timestamps, values, true);
    
    // Auto-range
    if (!timestamps.isEmpty()) {
        double xMin = timestamps.first();
        double xMax = timestamps.last();
        m_plot->xAxis->setRange(xMin, xMax + 0.1);
        
        double yMin = *std::min_element(values.begin(), values.end());
        double yMax = *std::max_element(values.begin(), values.end());
        double margin = (yMax - yMin) * 0.1;
        if (margin < 0.001) margin = 1.0;
        m_plot->yAxis->setRange(yMin - margin, yMax + margin);
    }
    
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void ChannelPlotWindow::clear()
{
    if (m_graph) {
        m_graph->data()->clear();
        m_plot->replot();
    }
}

void ChannelPlotWindow::closeEvent(QCloseEvent *event)
{
    emit reattachRequested(m_channelIndex);
    event->accept();
}
