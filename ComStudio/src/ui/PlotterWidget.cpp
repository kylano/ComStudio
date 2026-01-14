/**
 * @file PlotterWidget.cpp
 * @brief Implementation of PlotterWidget
 */

#include "ui/PlotterWidget.h"
#include "ui/ChannelPlotWindow.h"
#include "qcustomplot.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>

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
    
    // Pre-allocate pending data buffer
    m_pendingData.reserve(PENDING_DATA_RESERVE);
    
    // Update timer for efficient replotting - slower for better performance
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(50);  // 20 FPS for smoother performance
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
    
    toolbarLayout->addWidget(new QLabel(tr("Buffer:")));
    m_bufferLimitSpin = new QSpinBox();
    m_bufferLimitSpin->setRange(100, 10000);
    m_bufferLimitSpin->setValue(m_maxDataPoints);
    m_bufferLimitSpin->setSingleStep(500);
    m_bufferLimitSpin->setToolTip(tr("Max points per channel (lower = faster)"));
    connect(m_bufferLimitSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PlotterWidget::onBufferLimitChanged);
    toolbarLayout->addWidget(m_bufferLimitSpin);
    
    m_bufferStatusLabel = new QLabel();
    m_bufferStatusLabel->setToolTip(tr("Current buffer usage"));
    toolbarLayout->addWidget(m_bufferStatusLabel);
    
    // Downsample mode selector
    toolbarLayout->addWidget(new QLabel(tr("Sample:")));
    m_downsampleModeCombo = new QComboBox();
    m_downsampleModeCombo->addItem(tr("LTTB"), static_cast<int>(DownsampleMode::LTTB));
    m_downsampleModeCombo->addItem(tr("Min-Max"), static_cast<int>(DownsampleMode::MinMax));
    m_downsampleModeCombo->setToolTip(tr("LTTB: smooth trends\nMin-Max: catches all spikes"));
    m_downsampleModeCombo->setCurrentIndex(0);
    connect(m_downsampleModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PlotterWidget::onDownsampleModeChanged);
    toolbarLayout->addWidget(m_downsampleModeCombo);
    
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
    // PERFORMANCE: Try to enable OpenGL GPU acceleration
#ifdef QCUSTOMPLOT_USE_OPENGL
    m_plot->setOpenGl(true);
    qDebug() << "QCustomPlot OpenGL enabled";
#else
    // OpenGL not compiled in - use software rendering optimizations
    qDebug() << "QCustomPlot using software rendering (OpenGL not available)";
#endif
    
    // PERFORMANCE: Disable anti-aliasing globally for speed
    m_plot->setNotAntialiasedElements(QCP::aeAll);
    m_plot->setAntialiasedElements(QCP::aeNone);
    
    // PERFORMANCE: Use faster plotting mode - but NOT phImmediateRefresh to reduce flicker
    m_plot->setPlottingHints(QCP::phFastPolylines | QCP::phCacheLabels);
    
    // ANTI-FLICKER: Set buffer device pixel ratio for smoother rendering
    m_plot->setBufferDevicePixelRatio(1.0);
    
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
    
    // Enable legend item selection for pop-out
    m_plot->legend->setSelectableParts(QCPLegend::spItems);
    connect(m_plot, &QCustomPlot::legendDoubleClick, this, &PlotterWidget::onLegendClick);
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

bool PlotterWidget::setOpenGlEnabled(bool enabled)
{
#ifdef QCUSTOMPLOT_USE_OPENGL
    m_plot->setOpenGl(enabled);
    m_plot->replot();
    qDebug() << "QCustomPlot OpenGL" << (enabled ? "enabled" : "disabled");
    return true;
#else
    Q_UNUSED(enabled);
    qDebug() << "OpenGL support not compiled in";
    return false;
#endif
}

bool PlotterWidget::isOpenGlEnabled() const
{
#ifdef QCUSTOMPLOT_USE_OPENGL
    return m_plot->openGl();
#else
    return false;
#endif
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
    
    // Batch the data - actual channel storage happens in timer
    PendingData pd;
    pd.timestamp = time;
    pd.values = packet.values;
    m_pendingData.append(pd);
    
    m_needsReplot = true;
}

void PlotterWidget::clear()
{
    m_channelData.clear();
    m_pendingData.clear();
    m_pendingData.reserve(PENDING_DATA_RESERVE);
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
    // Process all pending data in batch
    if (!m_pendingData.isEmpty() && !m_paused) {
        for (const auto &pd : m_pendingData) {
            for (int i = 0; i < pd.values.size(); ++i) {
                auto &channelData = m_channelData[i];
                channelData.timestamps.append(pd.timestamp);
                channelData.values.append(pd.values[i]);
            }
        }
        m_pendingData.clear();
        m_pendingData.reserve(PENDING_DATA_RESERVE);  // Keep capacity
        
        // Trim data points in batch - use mid() which is faster than remove()
        for (auto it = m_channelData.begin(); it != m_channelData.end(); ++it) {
            auto &data = it.value();
            int currentSize = data.timestamps.size();
            if (currentSize > m_maxDataPoints) {
                // Keep only the last m_maxDataPoints entries
                int keepFrom = currentSize - m_maxDataPoints;
                data.timestamps = data.timestamps.mid(keepFrom);
                data.values = data.values.mid(keepFrom);
                // Re-reserve capacity to prevent frequent reallocations
                data.timestamps.reserve(m_maxDataPoints + 500);
                data.values.reserve(m_maxDataPoints + 500);
            }
        }
        
        // Ensure graphs exist for all channels
        for (auto it = m_channelData.begin(); it != m_channelData.end(); ++it) {
            ensureGraph(it.key());
        }
    }
    
    // Update buffer status indicator (less frequently)
    static int statusCounter = 0;
    if (++statusCounter >= 10) {  // Update every ~500ms at 20 FPS
        statusCounter = 0;
        int maxPoints = 0;
        for (const auto &data : m_channelData) {
            maxPoints = qMax(maxPoints, static_cast<int>(data.timestamps.size()));
        }
        
        if (maxPoints > 0) {
            int percent = (maxPoints * 100) / m_maxDataPoints;
            QString status = QString("%1/%2").arg(maxPoints).arg(m_maxDataPoints);
            if (percent >= 90) {
                m_bufferStatusLabel->setStyleSheet("color: #f38ba8;");
            } else if (percent >= 70) {
                m_bufferStatusLabel->setStyleSheet("color: #fab387;");
            } else {
                m_bufferStatusLabel->setStyleSheet("");
            }
            m_bufferStatusLabel->setText(status);
        } else {
            m_bufferStatusLabel->setText("");
            m_bufferStatusLabel->setStyleSheet("");
        }
    }
    
    if (!m_needsReplot || m_paused) {
        return;
    }
    
    // Update graph data with selected downsampling algorithm
    for (auto it = m_channelData.begin(); it != m_channelData.end(); ++it) {
        int channelIndex = it.key();
        const auto &data = it.value();
        
        if (channelIndex < m_graphs.size() && m_graphs[channelIndex]) {
            int dataSize = data.timestamps.size();
            
            // PERFORMANCE: Downsample if too many points
            if (dataSize > MAX_DISPLAY_POINTS) {
                QVector<double> dsTime, dsValue;
                
                if (m_downsampleMode == DownsampleMode::MinMax) {
                    // ============================================================
                    // MIN-MAX DOWNSAMPLING
                    // Best for catching spikes/glitches - preserves signal envelope
                    // ============================================================
                    int numBuckets = MAX_DISPLAY_POINTS / 2;
                    dsTime.reserve(numBuckets * 2 + 2);
                    dsValue.reserve(numBuckets * 2 + 2);
                    
                    // Always include first point
                    dsTime.append(data.timestamps.first());
                    dsValue.append(data.values.first());
                    
                    double bucketSize = static_cast<double>(dataSize - 2) / numBuckets;
                    
                    for (int bucket = 0; bucket < numBuckets; ++bucket) {
                        int startIdx = 1 + static_cast<int>(bucket * bucketSize);
                        int endIdx = 1 + static_cast<int>((bucket + 1) * bucketSize);
                        endIdx = qMin(endIdx, dataSize - 1);
                        
                        if (startIdx >= endIdx) continue;
                        
                        // Find min and max in this bucket
                        int minIdx = startIdx, maxIdx = startIdx;
                        double minVal = data.values[startIdx];
                        double maxVal = data.values[startIdx];
                        
                        for (int i = startIdx + 1; i < endIdx; ++i) {
                            if (data.values[i] < minVal) {
                                minVal = data.values[i];
                                minIdx = i;
                            }
                            if (data.values[i] > maxVal) {
                                maxVal = data.values[i];
                                maxIdx = i;
                            }
                        }
                        
                        // Add min and max in time order to preserve signal shape
                        if (minIdx <= maxIdx) {
                            dsTime.append(data.timestamps[minIdx]);
                            dsValue.append(minVal);
                            if (minIdx != maxIdx) {
                                dsTime.append(data.timestamps[maxIdx]);
                                dsValue.append(maxVal);
                            }
                        } else {
                            dsTime.append(data.timestamps[maxIdx]);
                            dsValue.append(maxVal);
                            dsTime.append(data.timestamps[minIdx]);
                            dsValue.append(minVal);
                        }
                    }
                    
                    // Always include last point
                    dsTime.append(data.timestamps.last());
                    dsValue.append(data.values.last());
                    
                } else {
                    // ============================================================
                    // LTTB DOWNSAMPLING (default)
                    // Best for smooth trend visualization - preserves visual shape
                    // ============================================================
                    dsTime.reserve(MAX_DISPLAY_POINTS);
                    dsValue.reserve(MAX_DISPLAY_POINTS);
                    
                    // Always include first point
                    dsTime.append(data.timestamps.first());
                    dsValue.append(data.values.first());
                    
                    // Bucket size (first and last points are fixed)
                    double bucketSize = static_cast<double>(dataSize - 2) / (MAX_DISPLAY_POINTS - 2);
                    
                    int prevSelectedIdx = 0;  // Index of last selected point
                    
                    for (int bucket = 0; bucket < MAX_DISPLAY_POINTS - 2; ++bucket) {
                        // Current bucket range
                        int bucketStart = static_cast<int>(bucket * bucketSize) + 1;
                        int bucketEnd = static_cast<int>((bucket + 1) * bucketSize) + 1;
                        bucketEnd = qMin(bucketEnd, dataSize - 1);
                        
                        // Next bucket range (for averaging)
                        int nextBucketStart = bucketEnd;
                        int nextBucketEnd = static_cast<int>((bucket + 2) * bucketSize) + 1;
                        nextBucketEnd = qMin(nextBucketEnd, dataSize);
                        
                        // Calculate average point of next bucket (point C in triangle)
                        double avgX = 0, avgY = 0;
                        int nextCount = nextBucketEnd - nextBucketStart;
                        if (nextCount > 0) {
                            for (int i = nextBucketStart; i < nextBucketEnd; ++i) {
                                avgX += data.timestamps[i];
                                avgY += data.values[i];
                            }
                            avgX /= nextCount;
                            avgY /= nextCount;
                        } else {
                            // Last bucket - use last point
                            avgX = data.timestamps.last();
                            avgY = data.values.last();
                        }
                        
                        // Point A (previously selected point)
                        double ax = data.timestamps[prevSelectedIdx];
                        double ay = data.values[prevSelectedIdx];
                        
                        // Find point in current bucket that maximizes triangle area
                        double maxArea = -1;
                        int maxAreaIdx = bucketStart;
                        
                        for (int i = bucketStart; i < bucketEnd; ++i) {
                            double bx = data.timestamps[i];
                            double by = data.values[i];
                            double area = qAbs((ax - avgX) * (by - ay) - (ax - bx) * (avgY - ay));
                            
                            if (area > maxArea) {
                                maxArea = area;
                                maxAreaIdx = i;
                            }
                        }
                        
                        dsTime.append(data.timestamps[maxAreaIdx]);
                        dsValue.append(data.values[maxAreaIdx]);
                        prevSelectedIdx = maxAreaIdx;
                    }
                    
                    // Always include last point
                    dsTime.append(data.timestamps.last());
                    dsValue.append(data.values.last());
                }
                
                m_graphs[channelIndex]->setData(dsTime, dsValue, true);
            } else {
                m_graphs[channelIndex]->setData(data.timestamps, data.values, true);
            }
        }
    }
    
    updateAxisRanges();
    
    // Use rpQueuedReplot for smoother updates (reduces flicker)
    // rpRefreshHint tells Qt to batch the repaint with other pending paints
    m_plot->replot(QCustomPlot::rpQueuedReplot);
    
    // Update detached windows (less frequently)
    static int windowCounter = 0;
    if (++windowCounter >= 3) {  // Every 3rd frame
        windowCounter = 0;
        for (auto it = m_detachedWindows.begin(); it != m_detachedWindows.end(); ++it) {
            int channelIndex = it.key();
            if (m_channelData.contains(channelIndex)) {
                const auto &data = m_channelData[channelIndex];
                it.value()->updateData(data.timestamps, data.values);
            }
        }
    }
    
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

void PlotterWidget::onBufferLimitChanged(int value)
{
    m_maxDataPoints = value;
    
    // Trim existing data if over new limit
    for (auto it = m_channelData.begin(); it != m_channelData.end(); ++it) {
        auto &data = it.value();
        while (data.timestamps.size() > m_maxDataPoints) {
            data.timestamps.removeFirst();
            data.values.removeFirst();
        }
    }
    
    m_needsReplot = true;
}

void PlotterWidget::onDownsampleModeChanged(int index)
{
    m_downsampleMode = static_cast<DownsampleMode>(m_downsampleModeCombo->itemData(index).toInt());
    m_needsReplot = true;
}

void PlotterWidget::onLegendClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event)
{
    Q_UNUSED(legend);
    Q_UNUSED(event);
    
    // Find which graph was clicked
    if (auto *plItem = qobject_cast<QCPPlottableLegendItem*>(item)) {
        for (int i = 0; i < m_graphs.size(); ++i) {
            if (m_graphs[i] == plItem->plottable()) {
                popOutChannel(i);
                break;
            }
        }
    }
}

void PlotterWidget::popOutChannel(int channelIndex)
{
    if (m_detachedChannels.contains(channelIndex)) {
        // Already detached, bring to front
        if (m_detachedWindows.contains(channelIndex)) {
            m_detachedWindows[channelIndex]->raise();
            m_detachedWindows[channelIndex]->activateWindow();
        }
        return;
    }
    
    if (channelIndex >= m_graphs.size()) {
        return;
    }
    
    QString name = m_graphs[channelIndex]->name();
    QColor color = channelColor(channelIndex);
    
    auto *window = new ChannelPlotWindow(channelIndex, name, color, nullptr);
    connect(window, &ChannelPlotWindow::reattachRequested,
            this, &PlotterWidget::onChannelReattach);
    
    m_detachedWindows[channelIndex] = window;
    m_detachedChannels.insert(channelIndex);
    
    // Hide from main plot
    m_graphs[channelIndex]->setVisible(false);
    
    // Send current data
    if (m_channelData.contains(channelIndex)) {
        const auto &data = m_channelData[channelIndex];
        window->updateData(data.timestamps, data.values);
    }
    
    window->show();
    m_needsReplot = true;
}

void PlotterWidget::onChannelReattach(int channelIndex)
{
    m_detachedChannels.remove(channelIndex);
    
    if (m_detachedWindows.contains(channelIndex)) {
        m_detachedWindows[channelIndex]->deleteLater();
        m_detachedWindows.remove(channelIndex);
    }
    
    // Show in main plot again
    if (channelIndex < m_graphs.size()) {
        m_graphs[channelIndex]->setVisible(true);
    }
    
    m_needsReplot = true;
}

QCPGraph* PlotterWidget::ensureGraph(int channelIndex)
{
    // Extend graphs vector if needed
    while (m_graphs.size() <= channelIndex) {
        QCPGraph *graph = m_plot->addGraph();
        graph->setName(QString("Ch%1").arg(m_graphs.size()));
        
        // Use SOLID pen - explicitly set to avoid any dashed appearance
        QPen pen(channelColor(m_graphs.size()));
        pen.setStyle(Qt::SolidLine);  // Ensure solid line, not dashed
        pen.setWidth(2);  // Slightly thicker for visibility
        pen.setCosmetic(true);  // Constant width regardless of zoom
        graph->setPen(pen);
        
        graph->setAntialiased(false);  // PERFORMANCE: Disable anti-aliasing
        graph->setAdaptiveSampling(true);  // PERFORMANCE: Enable adaptive sampling
        graph->setLineStyle(QCPGraph::lsLine);  // Simple connected line
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
    
    // Auto-scale Y axis - THROTTLED for performance
    if (m_autoScale) {
        // Only recalculate Y range every 5 frames (~250ms at 20 FPS)
        if (++m_autoScaleCounter >= 5) {
            m_autoScaleCounter = 0;
            
            double yMin = std::numeric_limits<double>::max();
            double yMax = std::numeric_limits<double>::lowest();
            
            // OPTIMIZATION: Sample only every Nth point instead of all points
            for (const auto &data : m_channelData) {
                int dataSize = data.timestamps.size();
                if (dataSize == 0) continue;
                
                // Find start index for visible window using binary search approximation
                int startIdx = 0;
                if (dataSize > 100) {
                    // Quick estimate: assume roughly uniform distribution
                    double firstTime = data.timestamps.first();
                    double lastTime = data.timestamps.last();
                    if (lastTime > firstTime) {
                        double ratio = (xMin - firstTime) / (lastTime - firstTime);
                        startIdx = qMax(0, qMin(dataSize - 1, static_cast<int>(ratio * dataSize)));
                    }
                }
                
                // Sample every Nth point for speed
                int step = qMax(1, (dataSize - startIdx) / 200);  // Max 200 samples
                for (int i = startIdx; i < dataSize; i += step) {
                    if (data.timestamps[i] >= xMin) {
                        yMin = qMin(yMin, data.values[i]);
                        yMax = qMax(yMax, data.values[i]);
                    }
                }
                // Always include the last point
                if (dataSize > 0) {
                    yMin = qMin(yMin, data.values.last());
                    yMax = qMax(yMax, data.values.last());
                }
            }
            
            if (yMin < yMax) {
                m_cachedYMin = yMin;
                m_cachedYMax = yMax;
            }
        }
        
        // Apply cached range
        if (m_cachedYMin < m_cachedYMax) {
            double margin = (m_cachedYMax - m_cachedYMin) * 0.1;
            m_plot->yAxis->setRange(m_cachedYMin - margin, m_cachedYMax + margin);
        }
    }
}

QColor PlotterWidget::channelColor(int index) const
{
    return s_channelColors[index % s_channelColors.size()];
}
