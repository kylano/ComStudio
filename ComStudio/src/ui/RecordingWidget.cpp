/**
 * @file RecordingWidget.cpp
 * @brief Implementation of RecordingWidget
 */

#include "ui/RecordingWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>

RecordingWidget::RecordingWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

RecordingWidget::~RecordingWidget()
{
    if (m_isRecording) {
        stopRecording();
    }
}

void RecordingWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // File path row
    auto *fileLayout = new QHBoxLayout();
    fileLayout->addWidget(new QLabel(tr("File:")));
    
    m_filePathEdit = new QLineEdit();
    m_filePathEdit->setPlaceholderText(tr("recording.csv"));
    m_filePathEdit->setText(QString("recording_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")));
    fileLayout->addWidget(m_filePathEdit, 1);
    
    m_browseButton = new QPushButton(tr("..."));
    m_browseButton->setFixedWidth(30);
    connect(m_browseButton, &QPushButton::clicked, this, &RecordingWidget::onBrowseClicked);
    fileLayout->addWidget(m_browseButton);
    
    mainLayout->addLayout(fileLayout);
    
    // Options row
    auto *optionsLayout = new QHBoxLayout();
    
    m_timestampCheck = new QCheckBox(tr("Include timestamp"));
    m_timestampCheck->setChecked(true);
    m_timestampCheck->setToolTip(tr("Add timestamp as first column"));
    optionsLayout->addWidget(m_timestampCheck);
    
    optionsLayout->addStretch();
    
    m_idFilterCheck = new QCheckBox(tr("Filter ID:"));
    m_idFilterCheck->setToolTip(tr("Only record packets with this sensor ID"));
    optionsLayout->addWidget(m_idFilterCheck);
    
    m_idFilterSpin = new QSpinBox();
    m_idFilterSpin->setRange(-1, 255);
    m_idFilterSpin->setValue(-1);
    m_idFilterSpin->setSpecialValueText(tr("Any"));
    m_idFilterSpin->setEnabled(false);
    connect(m_idFilterCheck, &QCheckBox::toggled, m_idFilterSpin, &QSpinBox::setEnabled);
    optionsLayout->addWidget(m_idFilterSpin);
    
    mainLayout->addLayout(optionsLayout);
    
    // Control row
    auto *controlLayout = new QHBoxLayout();
    
    m_startStopButton = new QPushButton(tr("Start Recording"));
    m_startStopButton->setCheckable(true);
    connect(m_startStopButton, &QPushButton::clicked, this, &RecordingWidget::onStartStopClicked);
    controlLayout->addWidget(m_startStopButton);
    
    m_statusLabel = new QLabel(tr("Ready"));
    controlLayout->addWidget(m_statusLabel, 1);
    
    mainLayout->addLayout(controlLayout);
    
    mainLayout->addStretch();
}

void RecordingWidget::onStartStopClicked()
{
    if (m_isRecording) {
        stopRecording();
        m_startStopButton->setChecked(false);
    } else {
        if (startRecording()) {
            m_startStopButton->setChecked(true);
        } else {
            m_startStopButton->setChecked(false);
        }
    }
}

void RecordingWidget::onBrowseClicked()
{
    QString path = QFileDialog::getSaveFileName(this,
        tr("Save Recording"),
        m_filePathEdit->text(),
        tr("CSV Files (*.csv);;All Files (*)"));
    
    if (!path.isEmpty()) {
        m_filePathEdit->setText(path);
    }
}

bool RecordingWidget::startRecording()
{
    QString path = m_filePathEdit->text();
    if (path.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please specify a file path."));
        return false;
    }
    
    m_file.setFileName(path);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Could not open file for writing:\n%1").arg(m_file.errorString()));
        return false;
    }
    
    m_stream.setDevice(&m_file);
    m_isRecording = true;
    m_headerWritten = false;
    m_recordCount = 0;
    m_maxChannelsSeen = 0;
    
    m_startStopButton->setText(tr("Stop Recording"));
    m_statusLabel->setText(tr("Recording..."));
    m_filePathEdit->setEnabled(false);
    m_browseButton->setEnabled(false);
    m_timestampCheck->setEnabled(false);
    m_idFilterCheck->setEnabled(false);
    m_idFilterSpin->setEnabled(false);
    
    return true;
}

void RecordingWidget::stopRecording()
{
    m_isRecording = false;
    m_stream.flush();
    m_file.close();
    
    m_startStopButton->setText(tr("Start Recording"));
    m_statusLabel->setText(tr("Saved %1 records").arg(m_recordCount));
    m_filePathEdit->setEnabled(true);
    m_browseButton->setEnabled(true);
    m_timestampCheck->setEnabled(true);
    m_idFilterCheck->setEnabled(true);
    m_idFilterSpin->setEnabled(m_idFilterCheck->isChecked());
}

void RecordingWidget::recordPacket(const GenericDataPacket &packet)
{
    if (!m_isRecording || !packet.isValid) {
        return;
    }
    
    // Apply ID filter
    if (m_idFilterCheck->isChecked()) {
        QString filterId = QString::number(m_idFilterSpin->value());
        // Compare as strings - handles alphanumeric IDs
        if (!filterId.isEmpty() && filterId != "-1" && packet.sensorId != filterId) {
            return;
        }
    }
    
    // Write header on first packet (to know channel count)
    if (!m_headerWritten) {
        m_maxChannelsSeen = packet.values.size();
        writeHeader();
        m_headerWritten = true;
    }
    
    // Expand header if more channels appear
    if (packet.values.size() > m_maxChannelsSeen) {
        m_maxChannelsSeen = packet.values.size();
        // Note: CSV header already written; new columns won't have headers
        // This is a trade-off for streaming writes
    }
    
    writePacket(packet);
    m_recordCount++;
    
    // Update status periodically
    if (m_recordCount % 100 == 0) {
        m_statusLabel->setText(tr("Recording... %1 records").arg(m_recordCount));
    }
}

void RecordingWidget::writeHeader()
{
    QStringList headers;
    
    if (m_timestampCheck->isChecked()) {
        headers.append("Timestamp");
    }
    
    headers.append("PacketIndex");
    headers.append("SensorID");
    
    for (int i = 0; i < m_maxChannelsSeen; ++i) {
        headers.append(QString("Ch%1").arg(i));
    }
    
    m_stream << headers.join(",") << "\n";
}

void RecordingWidget::writePacket(const GenericDataPacket &packet)
{
    QStringList values;
    
    if (m_timestampCheck->isChecked()) {
        values.append(QString::number(packet.timestamp));
    }
    
    values.append(QString::number(packet.packetIndex));
    values.append(packet.sensorId.isEmpty() ? "" : packet.sensorId);
    
    for (int i = 0; i < m_maxChannelsSeen; ++i) {
        if (i < packet.values.size()) {
            values.append(QString::number(packet.values[i], 'f', 6));
        } else {
            values.append("");
        }
    }
    
    m_stream << values.join(",") << "\n";
}
