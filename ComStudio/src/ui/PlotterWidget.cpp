/**
 * @file PlotterWidget.cpp
 * @brief Implementation of PlotterWidget
 */

#include "ui/PlotterWidget.h"
#include "qcustomplot.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QDateTime>

// Catppuccin Mocha color palette for channels
const QVector<QColor> PlotterWidget::s_channelColors = {
    QColor(137, 180, 250),  // Blue
    QColor(166, 227, 161),  // Green
    QColor(249, 226, 175),  // Yellow
    QColor(243, 139, 168),  // Red
    QColor(203, 166, 247),  // Mauve
    QColor(148, 226, 213),  // Teal
    QColor(250, 179, 135),  // Peach
    QColor(245, 194, 231),  // Pink
    QColor(180, 190, 254),  // Lavender
    QColor(116, 199, 236),  // Sapphire
};

PlotterWidget::PlotterWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupPlot();
    
    // Update timer for efficient replotting
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(33);  // ~30 FPS
    connect(m_updateTimer, &QTimer::timeout, this, &PlotterWidget::onUpdateTimer);
    m_updateTimer->start();
}

PlotterWidget::~PlotterWidget()
{
    // Cleanup handled by Qt parent-child system
}

void PlotterWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);
    
    // Toolbar
    auto *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);
    
    toolbarLayout->addWidget(new QLabel(tr("Window (s):")));
    m_timeWindowSpin = new QSpinBox();
    m_timeWindowSpin->setRange(1, 300);
    m_timeWindowSpin->setValue(static_cast<int>(m_timeWindow));
    m_timeWindowSpin->setSuffix(" s");
    connect(m_timeWindowSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PlotterWidget::onTimeWindowChanged);
    toolbarLayout->addWidget(m_timeWindowSpin);
    
    m_autoScaleCheck = new QCheckBox(tr("Auto-scale Y"));
    m_autoScaleCheck->setChecked(m_autoScale);
    connect(m_autoScaleCheck, &QCheckBox::toggled, this, &PlotterWidget::onAutoScaleToggled);
    toolbarLayout->addWidget(m_autoScaleCheck);
    
    toolbarLayout->addStretch();
    
    m_pauseButton = new QPushButton(tr("Pause"));
    m_pauseButton->setCheckable(true);
    connect(m_pauseButton, &QPushButton::clicked, this, &PlotterWidget::onPauseClicked);
    toolbarLayout->addWidget(m_pauseButton);
    
    m_clearButton = new QPushButton(tr("Clear"));
    connect(m_clearButton, &QPushButton::clicked, this, &PlotterWidget::onClearClicked);
    toolbarLayout->addWidget(m_clearButton);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Plot widget
    m_plot = new QCustomPlot();
    m_plot->setMinimumHeight(200);
    mainLayout->addWidget(m_plot, 1);
}

void PlotterWidget::setupPlot()
{
    // Configure dark theme
    m_plot->setBackground(QBrush(QColor(0x1e, 0x1e, 0x2e)));
    
    // X Axis (Time)
    m_plot->xAxis->setBasePen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->xAxis->setTickPen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->xAxis->setSubTickPen(QPen(QColor(0x31, 0x32, 0x44)));
    m_plot->xAxis->setTickLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->xAxis->setLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->xAxis->setLabel("Time (s)");
    m_plot->xAxis->grid()->setPen(QPen(QColor(0x31, 0x32, 0x44), 1, Qt::DotLine));
    
    // Y Axis (Value)
    m_plot->yAxis->setBasePen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->yAxis->setTickPen(QPen(QColor(0x45, 0x47, 0x5a)));
    m_plot->yAxis->setSubTickPen(QPen(QColor(0x31, 0x32, 0x44)));
    m_plot->yAxis->setTickLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->yAxis->setLabelColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->yAxis->setLabel("Value");
    m_plot->yAxis->grid()->setPen(QPen(QColor(0x31, 0x32, 0x44), 1, Qt::DotLine));
    
    // Interactions
    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    m_plot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    m_plot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
    
    // Legend
    m_plot->legend->setVisible(true);
    m_plot->legend->setBrush(QBrush(QColor(0x18, 0x18, 0x25, 200)));
    m_plot->legend->setTextColor(QColor(0xcd, 0xd6, 0xf4));
    m_plot->legend->setBorderPen(QPen(QColor(0x31, 0x32, 0x44)));
    m_plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);
}

void PlotterWidget::setTimeWindow(double seconds)
{
    m_timeWindow = seconds;
    m_timeWindowSpin->setValue(static_cast<int>(seconds));
}

void PlotterWidget::setMaxDataPoints(int points)
{
    m_maxDataPoints = points;
}

void PlotterWidget::setAutoScale(bool enabled)
{
    m_autoScale = enabled;
    m_autoScaleCheck->setChecked(enabled);
}

void PlotterWidget::addData(const GenericDataPacket &packet)
{
    if (m_paused || !packet.isValid) {
        return;
    }
    
    // Initialize start time on first data
    if (m_startTime == 0) {
        m_startTime = packet.timestamp;
    }
    
    // Convert timestamp to seconds from start
    double time = (packet.timestamp - m_startTime) / 1000.0;
    
    // Add data for each channel
    for (int i = 0; i < packet.values.size(); ++i) {
        auto &channelData = m_channelData[i];
        
        channelData.timestamps.append(time);
        channelData.values.append(packet.values[i]);
        
        // Limit data points
        while (channelData.timestamps.size() > m_maxDataPoints) {
            channelData.timestamps.removeFirst();
            channelData.values.removeFirst();
        }
        
        // Ensure graph exists
        ensureGraph(i);
    }
    
    m_needsReplot = true;
}

void PlotterWidget::clear()
{
    m_channelData.clear();
    m_startTime = 0;
    
    for (auto *graph : m_graphs) {
        graph->data()->clear();
    }
    
    m_plot->replot();
}

void PlotterWidget::setPaused(bool paused)
{
    m_paused = paused;
    m_pauseButton->setChecked(paused);
    m_pauseButton->setText(paused ? tr("Resume") : tr("Pause"));
}

void PlotterWidget::onUpdateTimer()
{
    if (!m_needsReplot || m_paused) {
        return;
    }
    
    // Update graph data
    for (auto it = m_channelData.begin(); it != m_channelData.end(); ++it) {
        int channelIndex = it.key();
        const auto &data = it.value();
        
        if (channelIndex < m_graphs.size() && m_graphs[channelIndex]) {
            m_graphs[channelIndex]->setData(data.timestamps, data.values, true);
        }
    }
    
    updateAxisRanges();
    m_plot->replot(QCustomPlot::rpQueuedReplot);
    m_needsReplot = false;
}

void PlotterWidget::onPauseClicked()
{
    setPaused(m_pauseButton->isChecked());
}

void PlotterWidget::onClearClicked()
{
    clear();
}

void PlotterWidget::onTimeWindowChanged(int value)
{
    m_timeWindow = value;
}

void PlotterWidget::onAutoScaleToggled(bool checked)
{
    m_autoScale = checked;
}

QCPGraph* PlotterWidget::ensureGraph(int channelIndex)
{
    // Extend graphs vector if needed
    while (m_graphs.size() <= channelIndex) {
        QCPGraph *graph = m_plot->addGraph();
        graph->setName(QString("Ch%1").arg(m_graphs.size()));
        graph->setPen(QPen(channelColor(m_graphs.size()), 2));
        graph->setAntialiased(true);
        m_graphs.append(graph);
    }
    
    return m_graphs[channelIndex];
}

void PlotterWidget::updateAxisRanges()
{
    if (m_channelData.isEmpty()) {
        return;
    }
    
    // Find current time (latest timestamp)
    double currentTime = 0;
    for (const auto &data : m_channelData) {
        if (!data.timestamps.isEmpty()) {
            currentTime = qMax(currentTime, data.timestamps.last());
        }
    }
    
    // Set X range to show time window
    double xMin = qMax(0.0, currentTime - m_timeWindow);
    m_plot->xAxis->setRange(xMin, currentTime + 0.1);
    
    // Auto-scale Y axis
    if (m_autoScale) {
        double yMin = std::numeric_limits<double>::max();
        double yMax = std::numeric_limits<double>::lowest();
        
        for (const auto &data : m_channelData) {
            // Only consider visible data points
            for (int i = 0; i < data.timestamps.size(); ++i) {
                if (data.timestamps[i] >= xMin) {
                    yMin = qMin(yMin, data.values[i]);
                    yMax = qMax(yMax, data.values[i]);
                }
            }
        }
        
        if (yMin < yMax) {
            double margin = (yMax - yMin) * 0.1;
            m_plot->yAxis->setRange(yMin - margin, yMax + margin);
        }
    }
}

QColor PlotterWidget::channelColor(int index) const
{
    return s_channelColors[index % s_channelColors.size()];
}
