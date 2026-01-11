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
    
    // Send input row
    auto *sendLayout = new QHBoxLayout();
    sendLayout->setSpacing(4);
    
    m_sendInput = new QLineEdit();
    m_sendInput->setPlaceholderText(tr("Enter data to send..."));
    connect(m_sendInput, &QLineEdit::returnPressed, this, &TerminalWidget::onSendClicked);
    sendLayout->addWidget(m_sendInput, 1);
    
    // Send mode selector (ASCII/Hex)
    m_sendModeCombo = new QComboBox();
    m_sendModeCombo->setToolTip(tr("Send mode"));
    m_sendModeCombo->addItem(tr("ASCII"), static_cast<int>(SendMode::ASCII));
    m_sendModeCombo->addItem(tr("Hex"), static_cast<int>(SendMode::Hex));
    m_sendModeCombo->setFixedWidth(70);
    sendLayout->addWidget(m_sendModeCombo);
    
    // Line ending selector
    m_lineEndingCombo = new QComboBox();
    m_lineEndingCombo->setToolTip(tr("Line ending"));
    m_lineEndingCombo->addItem(tr("None"), static_cast<int>(LineEnding::None));
    m_lineEndingCombo->addItem(tr("LF"), static_cast<int>(LineEnding::LF));
    m_lineEndingCombo->addItem(tr("CRLF"), static_cast<int>(LineEnding::CRLF));
    m_lineEndingCombo->setCurrentIndex(1);  // Default to LF
    m_lineEndingCombo->setFixedWidth(60);
    sendLayout->addWidget(m_lineEndingCombo);
    
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
    
    // Get current send mode
    SendMode mode = static_cast<SendMode>(m_sendModeCombo->currentData().toInt());
    LineEnding ending = static_cast<LineEnding>(m_lineEndingCombo->currentData().toInt());
    
    if (mode == SendMode::Hex) {
        // Parse as hex (strip 0x prefixes and spaces)
        QString cleanHex = text;
        cleanHex.remove("0x", Qt::CaseInsensitive);
        cleanHex.remove(' ');
        data = QByteArray::fromHex(cleanHex.toLatin1());
    } else {
        // Send as ASCII
        data = text.toUtf8();
    }
    
    // Append line ending (for both modes, user may want \n after hex)
    switch (ending) {
        case LineEnding::LF:
            data.append('\n');
            break;
        case LineEnding::CRLF:
            data.append("\r\n");
            break;
        case LineEnding::None:
        default:
            break;
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
