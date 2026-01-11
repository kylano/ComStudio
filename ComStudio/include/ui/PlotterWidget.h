/**
 * @file PlotterWidget.h
 * @brief Real-time data plotter using QCustomPlot
 *
 * Provides a real-time scrolling plot for visualizing
 * multi-channel serial data.
 */

#ifndef PLOTTERWIDGET_H
#define PLOTTERWIDGET_H

#include <QWidget>
#include <QVector>
#include <QTimer>
#include <QMap>
#include <QSet>

#include "core/GenericDataPacket.h"

class ChannelPlotWindow;

// Forward declarations
class QCustomPlot;
class QCPGraph;
class QCPLegend;
class QCPAbstractLegendItem;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QComboBox;
class QLabel;

/**
 * @class PlotterWidget
 * @brief Real-time data plotter widget
 *
 * Uses QCustomPlot to display time-series data with support
 * for multiple channels, auto-scaling, and configurable
 * display window.
 */
class PlotterWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit PlotterWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~PlotterWidget() override;
    
    /**
     * @brief Set the time window for display (in seconds)
     * @param seconds Time window in seconds
     */
    void setTimeWindow(double seconds);
    
    /**
     * @brief Set maximum number of data points per channel
     * @param points Maximum points
     */
    void setMaxDataPoints(int points);
    
    /**
     * @brief Enable/disable auto-scaling
     * @param enabled True for auto-scaling
     */
    void setAutoScale(bool enabled);

public slots:
    /**
     * @brief Add new data packet to plot
     * @param packet Parsed data packet
     */
    void addData(const GenericDataPacket &packet);
    
    /**
     * @brief Clear all plot data
     */
    void clear();
    
    /**
     * @brief Pause/resume plotting
     * @param paused True to pause
     */
    void setPaused(bool paused);

signals:
    /**
     * @brief Emitted when plot is clicked
     * @param x X coordinate (time)
     * @param y Y coordinate (value)
     */
    void plotClicked(double x, double y);

private slots:
    /**
     * @brief Handle plot update timer
     */
    void onUpdateTimer();
    
    /**
     * @brief Handle pause button click
     */
    void onPauseClicked();
    
    /**
     * @brief Handle clear button click
     */
    void onClearClicked();
    
    /**
     * @brief Handle time window change
     * @param value New time window in seconds
     */
    void onTimeWindowChanged(int value);
    
    /**
     * @brief Handle auto-scale toggle
     * @param checked Auto-scale state
     */
    void onAutoScaleToggled(bool checked);
    
    /**
     * @brief Handle buffer limit change
     * @param value New buffer limit
     */
    void onBufferLimitChanged(int value);
    
    /**
     * @brief Handle legend item click for pop-out
     * @param legend Legend clicked
     * @param item Item clicked
     */
    void onLegendClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent *event);
    
    /**
     * @brief Handle channel reattach request
     * @param channelIndex Channel to reattach
     */
    void onChannelReattach(int channelIndex);

private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Set up the plot
     */
    void setupPlot();
    
    /**
     * @brief Ensure graph exists for channel
     * @param channelIndex Channel index
     * @return Graph for the channel
     */
    QCPGraph* ensureGraph(int channelIndex);
    
    /**
     * @brief Update axis ranges
     */
    void updateAxisRanges();
    
    /**
     * @brief Get color for channel index
     * @param index Channel index
     * @return Color for the channel
     */
    QColor channelColor(int index) const;

    QCustomPlot *m_plot = nullptr;
    QSpinBox *m_timeWindowSpin = nullptr;
    QSpinBox *m_bufferLimitSpin = nullptr;
    QCheckBox *m_autoScaleCheck = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QLabel *m_bufferStatusLabel = nullptr;
    
    QTimer *m_updateTimer = nullptr;
    
    QVector<QCPGraph*> m_graphs;
    
    // Data storage per channel (for high-frequency updates)
    struct ChannelData {
        QVector<double> timestamps;
        QVector<double> values;
    };
    QMap<int, ChannelData> m_channelData;
    
    double m_timeWindow = 10.0;      ///< Display window in seconds
    int m_maxDataPoints = 10000;     ///< Max points per channel
    bool m_autoScale = true;
    bool m_paused = false;
    qint64 m_startTime = 0;          ///< First data timestamp
    bool m_needsReplot = false;
    
    // Channel colors (Catppuccin Mocha palette)
    static const QVector<QColor> s_channelColors;
    
    // Detached channel windows
    QMap<int, ChannelPlotWindow*> m_detachedWindows;
    QSet<int> m_detachedChannels;  // Channels that are popped out
    
    void popOutChannel(int channelIndex);
};

#endif // PLOTTERWIDGET_H
