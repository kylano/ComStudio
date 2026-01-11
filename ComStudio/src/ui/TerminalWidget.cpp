/**
 * @file TerminalWidget.cpp
 * @brief Implementation of TerminalWidget
 */

#include "ui/TerminalWidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDateTime>
#include <QLabel>

TerminalWidget::TerminalWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void TerminalWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);
    
    // Toolbar
    auto *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);
    
    toolbarLayout->addWidget(new QLabel(tr("Display:")));
    m_displayModeCombo = new QComboBox();
    m_displayModeCombo->addItem(tr("Raw"), static_cast<int>(DisplayMode::Raw));
    m_displayModeCombo->addItem(tr("Hex"), static_cast<int>(DisplayMode::Hex));
    m_displayModeCombo->addItem(tr("Parsed"), static_cast<int>(DisplayMode::Parsed));
    connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TerminalWidget::onDisplayModeChanged);
    toolbarLayout->addWidget(m_displayModeCombo);
    
    m_timestampCheck = new QCheckBox(tr("Timestamps"));
    m_timestampCheck->setChecked(false);
    toolbarLayout->addWidget(m_timestampCheck);
    
    m_autoScrollCheck = new QCheckBox(tr("Auto-scroll"));
    m_autoScrollCheck->setChecked(true);
    connect(m_autoScrollCheck, &QCheckBox::toggled, this, &TerminalWidget::onAutoScrollToggled);
    toolbarLayout->addWidget(m_autoScrollCheck);
    
    toolbarLayout->addStretch();
    
    m_clearButton = new QPushButton(tr("Clear"));
    connect(m_clearButton, &QPushButton::clicked, this, &TerminalWidget::clear);
    toolbarLayout->addWidget(m_clearButton);
    
    mainLayout->addLayout(toolbarLayout);
    
    // Terminal text area
    m_terminal = new QPlainTextEdit();
    m_terminal->setReadOnly(true);
    m_terminal->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_terminal->setMaximumBlockCount(m_maxLines);
    mainLayout->addWidget(m_terminal, 1);
    
    // Send input
    auto *sendLayout = new QHBoxLayout();
    sendLayout->setSpacing(4);
    
    m_sendInput = new QLineEdit();
    m_sendInput->setPlaceholderText(tr("Enter data to send..."));
    connect(m_sendInput, &QLineEdit::returnPressed, this, &TerminalWidget::onSendClicked);
    sendLayout->addWidget(m_sendInput, 1);
    
    m_sendButton = new QPushButton(tr("Send"));
    connect(m_sendButton, &QPushButton::clicked, this, &TerminalWidget::onSendClicked);
    sendLayout->addWidget(m_sendButton);
    
    mainLayout->addLayout(sendLayout);
}

void TerminalWidget::setDisplayMode(DisplayMode mode)
{
    m_displayMode = mode;
    
    // Update combo box
    for (int i = 0; i < m_displayModeCombo->count(); ++i) {
        if (m_displayModeCombo->itemData(i).toInt() == static_cast<int>(mode)) {
            m_displayModeCombo->setCurrentIndex(i);
            break;
        }
    }
}

void TerminalWidget::setMaxLines(int maxLines)
{
    m_maxLines = maxLines;
    m_terminal->setMaximumBlockCount(maxLines);
}

void TerminalWidget::appendRawData(const QByteArray &data)
{
    if (m_displayMode == DisplayMode::Parsed) {
        return;  // In parsed mode, only show parsed packets
    }
    
    QString formatted = formatData(data);
    
    if (m_timestampCheck->isChecked()) {
        QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss.zzz] ");
        formatted = timestamp + formatted;
    }
    
    m_terminal->appendPlainText(formatted);
    
    if (m_autoScroll) {
        m_terminal->verticalScrollBar()->setValue(m_terminal->verticalScrollBar()->maximum());
    }
}

void TerminalWidget::appendPacket(const GenericDataPacket &packet)
{
    QString formatted;
    
    if (m_displayMode == DisplayMode::Parsed) {
        formatted = formatPacket(packet);
    } else {
        formatted = formatData(packet.rawData);
    }
    
    if (m_timestampCheck->isChecked()) {
        QString timestamp = QDateTime::fromMSecsSinceEpoch(packet.timestamp)
            .toString("[hh:mm:ss.zzz] ");
        formatted = timestamp + formatted;
    }
    
    m_terminal->appendPlainText(formatted);
    
    if (m_autoScroll) {
        m_terminal->verticalScrollBar()->setValue(m_terminal->verticalScrollBar()->maximum());
    }
}

void TerminalWidget::clear()
{
    m_terminal->clear();
}

void TerminalWidget::onDisplayModeChanged(int index)
{
    m_displayMode = static_cast<DisplayMode>(m_displayModeCombo->itemData(index).toInt());
}

void TerminalWidget::onSendClicked()
{
    QString text = m_sendInput->text();
    if (text.isEmpty()) {
        return;
    }
    
    QByteArray data;
    
    // Check if input is hex format (starts with 0x or contains spaces between hex bytes)
    if (text.startsWith("0x", Qt::CaseInsensitive) || 
        text.contains(QRegularExpression("^[0-9A-Fa-f]{2}(\\s+[0-9A-Fa-f]{2})+$"))) {
        // Parse as hex
        QString cleanHex = text.remove("0x", Qt::CaseInsensitive).remove(' ');
        data = QByteArray::fromHex(cleanHex.toLatin1());
    } else {
        // Send as ASCII with newline
        data = (text + "\n").toUtf8();
    }
    
    emit sendData(data);
    m_sendInput->clear();
}

void TerminalWidget::onAutoScrollToggled(bool enabled)
{
    m_autoScroll = enabled;
}

QString TerminalWidget::formatData(const QByteArray &data) const
{
    switch (m_displayMode) {
        case DisplayMode::Hex:
            return data.toHex(' ').toUpper();
        case DisplayMode::Raw:
        case DisplayMode::Parsed:
        default:
            return QString::fromUtf8(data).trimmed();
    }
}

QString TerminalWidget::formatPacket(const GenericDataPacket &packet) const
{
    if (!packet.isValid) {
        return QString("[ERROR] %1").arg(packet.errorMessage);
    }
    
    QStringList parts;
    parts.append(QString("#%1").arg(packet.packetIndex));
    
    if (packet.sensorId >= 0) {
        parts.append(QString("ID:%1").arg(packet.sensorId));
    }
    
    for (auto it = packet.channels.constBegin(); it != packet.channels.constEnd(); ++it) {
        parts.append(QString("%1=%2").arg(it.key()).arg(it.value(), 0, 'f', 4));
    }
    
    return parts.join(" | ");
}

void TerminalWidget::trimLines()
{
    // QPlainTextEdit handles this automatically with setMaximumBlockCount
}
