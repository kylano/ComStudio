/**
 * @file RecordingWidget.h
 * @brief Widget for CSV recording controls
 */

#ifndef RECORDINGWIDGET_H
#define RECORDINGWIDGET_H

#include <QWidget>
#include <QFile>
#include <QTextStream>
#include <QVector>

#include "core/GenericDataPacket.h"

class QPushButton;
class QCheckBox;
class QLineEdit;
class QLabel;
class QSpinBox;

/**
 * @class RecordingWidget
 * @brief Widget for controlling CSV recording of parsed data
 * 
 * Provides controls to start/stop recording, configure timestamp
 * inclusion, ID filtering, and channel selection.
 */
class RecordingWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit RecordingWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~RecordingWidget() override;
    
    /**
     * @brief Check if recording is active
     * @return True if recording
     */
    bool isRecording() const { return m_isRecording; }

public slots:
    /**
     * @brief Add a packet to the recording (if active)
     * @param packet Parsed data packet
     */
    void recordPacket(const GenericDataPacket &packet);

private slots:
    void onStartStopClicked();
    void onBrowseClicked();

private:
    void setupUi();
    bool startRecording();
    void stopRecording();
    void writeHeader();
    void writePacket(const GenericDataPacket &packet);
    
    QPushButton *m_startStopButton = nullptr;
    QPushButton *m_browseButton = nullptr;
    QLineEdit *m_filePathEdit = nullptr;
    QCheckBox *m_timestampCheck = nullptr;
    QCheckBox *m_idFilterCheck = nullptr;
    QSpinBox *m_idFilterSpin = nullptr;
    QLabel *m_statusLabel = nullptr;
    
    QFile m_file;
    QTextStream m_stream;
    bool m_isRecording = false;
    bool m_headerWritten = false;
    int m_recordCount = 0;
    int m_maxChannelsSeen = 0;
};

#endif // RECORDINGWIDGET_H
