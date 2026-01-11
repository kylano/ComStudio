/**
 * @file ChannelPlotWindow.h
 * @brief Detachable window for a single plot channel
 */

#ifndef CHANNELPLOTWINDOW_H
#define CHANNELPLOTWINDOW_H

#include <QWidget>
#include <QVector>

class QCustomPlot;
class QCPGraph;

/**
 * @class ChannelPlotWindow
 * @brief A detachable window that displays a single channel's plot
 * 
 * This window can be popped out from the main plotter and displays
 * data for a specific channel. It receives data updates from the
 * main plotter widget.
 */
class ChannelPlotWindow : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param channelIndex Index of the channel to display
     * @param channelName Display name of the channel
     * @param color Color for the plot line
     * @param parent Parent widget
     */
    explicit ChannelPlotWindow(int channelIndex, const QString &channelName,
                               const QColor &color, QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ChannelPlotWindow() override;
    
    /**
     * @brief Get the channel index
     * @return Channel index
     */
    int channelIndex() const { return m_channelIndex; }

public slots:
    /**
     * @brief Update the plot with new data
     * @param timestamps Vector of timestamps
     * @param values Vector of values
     */
    void updateData(const QVector<double> &timestamps, const QVector<double> &values);
    
    /**
     * @brief Clear the plot
     */
    void clear();

signals:
    /**
     * @brief Emitted when window is about to close (user requests reattach)
     * @param channelIndex The channel index to reattach
     */
    void reattachRequested(int channelIndex);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupPlot();
    
    int m_channelIndex;
    QString m_channelName;
    QColor m_color;
    
    QCustomPlot *m_plot = nullptr;
    QCPGraph *m_graph = nullptr;
};

#endif // CHANNELPLOTWINDOW_H
