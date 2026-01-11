/**
 * @file SerialSettingsWidget.cpp
 * @brief Implementation of SerialSettingsWidget
 */

#include "ui/SerialSettingsWidget.h"

#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSerialPortInfo>

SerialSettingsWidget::SerialSettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    populateOptions();
    refreshPorts();
}

void SerialSettingsWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);
    
    // Port selection group
    auto *portGroup = new QGroupBox(tr("Port Settings"));
    auto *portLayout = new QGridLayout(portGroup);
    portLayout->setSpacing(8);
    
    // Port
    portLayout->addWidget(new QLabel(tr("Port:")), 0, 0);
    m_portCombo = new QComboBox();
    m_portCombo->setMinimumWidth(150);
    portLayout->addWidget(m_portCombo, 0, 1);
    
    m_refreshButton = new QPushButton(tr("Refresh"));
    m_refreshButton->setFixedWidth(80);
    connect(m_refreshButton, &QPushButton::clicked, this, &SerialSettingsWidget::onRefreshClicked);
    portLayout->addWidget(m_refreshButton, 0, 2);
    
    // Baud Rate
    portLayout->addWidget(new QLabel(tr("Baud Rate:")), 1, 0);
    m_baudRateCombo = new QComboBox();
    portLayout->addWidget(m_baudRateCombo, 1, 1, 1, 2);
    
    // Data Bits
    portLayout->addWidget(new QLabel(tr("Data Bits:")), 2, 0);
    m_dataBitsCombo = new QComboBox();
    portLayout->addWidget(m_dataBitsCombo, 2, 1, 1, 2);
    
    // Parity
    portLayout->addWidget(new QLabel(tr("Parity:")), 3, 0);
    m_parityCombo = new QComboBox();
    portLayout->addWidget(m_parityCombo, 3, 1, 1, 2);
    
    // Stop Bits
    portLayout->addWidget(new QLabel(tr("Stop Bits:")), 4, 0);
    m_stopBitsCombo = new QComboBox();
    portLayout->addWidget(m_stopBitsCombo, 4, 1, 1, 2);
    
    // Flow Control
    portLayout->addWidget(new QLabel(tr("Flow Control:")), 5, 0);
    m_flowControlCombo = new QComboBox();
    portLayout->addWidget(m_flowControlCombo, 5, 1, 1, 2);
    
    mainLayout->addWidget(portGroup);
    
    // Status label
    m_statusLabel = new QLabel(tr("Disconnected"));
    m_statusLabel->setObjectName("statusDisconnected");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);
    
    // Buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);
    
    m_connectButton = new QPushButton(tr("Connect"));
    m_connectButton->setObjectName("connectButton");
    m_connectButton->setMinimumHeight(36);
    connect(m_connectButton, &QPushButton::clicked, this, &SerialSettingsWidget::onConnectClicked);
    buttonLayout->addWidget(m_connectButton);
    
    m_disconnectButton = new QPushButton(tr("Disconnect"));
    m_disconnectButton->setObjectName("disconnectButton");
    m_disconnectButton->setMinimumHeight(36);
    m_disconnectButton->setEnabled(false);
    connect(m_disconnectButton, &QPushButton::clicked, this, &SerialSettingsWidget::onDisconnectClicked);
    buttonLayout->addWidget(m_disconnectButton);
    
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
}

void SerialSettingsWidget::populateOptions()
{
    // Baud rates
    m_baudRateCombo->addItem("9600", 9600);
    m_baudRateCombo->addItem("19200", 19200);
    m_baudRateCombo->addItem("38400", 38400);
    m_baudRateCombo->addItem("57600", 57600);
    m_baudRateCombo->addItem("115200", 115200);
    m_baudRateCombo->addItem("230400", 230400);
    m_baudRateCombo->addItem("460800", 460800);
    m_baudRateCombo->addItem("921600", 921600);
    m_baudRateCombo->setCurrentIndex(4);  // Default 115200
    
    // Data bits
    m_dataBitsCombo->addItem("5", QSerialPort::Data5);
    m_dataBitsCombo->addItem("6", QSerialPort::Data6);
    m_dataBitsCombo->addItem("7", QSerialPort::Data7);
    m_dataBitsCombo->addItem("8", QSerialPort::Data8);
    m_dataBitsCombo->setCurrentIndex(3);  // Default 8
    
    // Parity
    m_parityCombo->addItem("None", QSerialPort::NoParity);
    m_parityCombo->addItem("Even", QSerialPort::EvenParity);
    m_parityCombo->addItem("Odd", QSerialPort::OddParity);
    m_parityCombo->addItem("Space", QSerialPort::SpaceParity);
    m_parityCombo->addItem("Mark", QSerialPort::MarkParity);
    
    // Stop bits
    m_stopBitsCombo->addItem("1", QSerialPort::OneStop);
    m_stopBitsCombo->addItem("1.5", QSerialPort::OneAndHalfStop);
    m_stopBitsCombo->addItem("2", QSerialPort::TwoStop);
    
    // Flow control
    m_flowControlCombo->addItem("None", QSerialPort::NoFlowControl);
    m_flowControlCombo->addItem("Hardware (RTS/CTS)", QSerialPort::HardwareControl);
    m_flowControlCombo->addItem("Software (XON/XOFF)", QSerialPort::SoftwareControl);
}

void SerialSettingsWidget::refreshPorts()
{
    QString currentPort = m_portCombo->currentText();
    m_portCombo->clear();
    
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &port : ports) {
        QString displayName = QString("%1 - %2")
            .arg(port.portName())
            .arg(port.description().isEmpty() ? tr("Unknown") : port.description());
        m_portCombo->addItem(displayName, port.portName());
    }
    
    // Try to restore previous selection
    for (int i = 0; i < m_portCombo->count(); ++i) {
        if (m_portCombo->itemData(i).toString() == currentPort) {
            m_portCombo->setCurrentIndex(i);
            break;
        }
    }
}

SerialSettings SerialSettingsWidget::currentSettings() const
{
    SerialSettings settings;
    settings.portName = m_portCombo->currentData().toString();
    settings.baudRate = m_baudRateCombo->currentData().toInt();
    settings.dataBits = static_cast<QSerialPort::DataBits>(m_dataBitsCombo->currentData().toInt());
    settings.parity = static_cast<QSerialPort::Parity>(m_parityCombo->currentData().toInt());
    settings.stopBits = static_cast<QSerialPort::StopBits>(m_stopBitsCombo->currentData().toInt());
    settings.flowControl = static_cast<QSerialPort::FlowControl>(m_flowControlCombo->currentData().toInt());
    return settings;
}

void SerialSettingsWidget::onConnectionStateChanged(bool connected, const QString &message)
{
    updateConnectionState(connected);
    m_statusLabel->setText(message);
}

void SerialSettingsWidget::updateConnectionState(bool connected)
{
    m_isConnected = connected;
    
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_portCombo->setEnabled(!connected);
    m_baudRateCombo->setEnabled(!connected);
    m_dataBitsCombo->setEnabled(!connected);
    m_parityCombo->setEnabled(!connected);
    m_stopBitsCombo->setEnabled(!connected);
    m_flowControlCombo->setEnabled(!connected);
    m_refreshButton->setEnabled(!connected);
    
    m_statusLabel->setObjectName(connected ? "statusConnected" : "statusDisconnected");
    m_statusLabel->style()->unpolish(m_statusLabel);
    m_statusLabel->style()->polish(m_statusLabel);
}

void SerialSettingsWidget::onConnectClicked()
{
    emit connectRequested(currentSettings());
}

void SerialSettingsWidget::onDisconnectClicked()
{
    emit disconnectRequested();
}

void SerialSettingsWidget::onRefreshClicked()
{
    refreshPorts();
}
