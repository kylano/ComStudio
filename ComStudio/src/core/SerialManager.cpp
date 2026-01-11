/**
 * @file SerialManager.cpp
 * @brief Implementation of SerialManager and SerialWorker classes
 */

#include "core/SerialManager.h"
#include <QDebug>

// ============================================================================
// SerialWorker Implementation
// ============================================================================

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
{
}

SerialWorker::~SerialWorker()
{
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }
}

void SerialWorker::openPort(const SerialSettings &settings)
{
    // Close existing port if open
    if (m_serialPort && m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    
    m_serialPort = std::make_unique<QSerialPort>();
    
    // Configure port
    m_serialPort->setPortName(settings.portName);
    m_serialPort->setBaudRate(settings.baudRate);
    m_serialPort->setDataBits(settings.dataBits);
    m_serialPort->setParity(settings.parity);
    m_serialPort->setStopBits(settings.stopBits);
    m_serialPort->setFlowControl(settings.flowControl);
    
    // Connect signals
    connect(m_serialPort.get(), &QSerialPort::readyRead,
            this, &SerialWorker::handleReadyRead);
    connect(m_serialPort.get(), &QSerialPort::errorOccurred,
            this, &SerialWorker::handleError);
    
    // Attempt to open
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_readBuffer.clear();
        emit connectionStateChanged(true, 
            QString("Connected to %1 @ %2 baud")
                .arg(settings.portName)
                .arg(settings.baudRate));
    } else {
        emit connectionStateChanged(false, 
            QString("Failed to open %1: %2")
                .arg(settings.portName)
                .arg(m_serialPort->errorString()));
        m_serialPort.reset();
    }
}

void SerialWorker::closePort()
{
    if (m_serialPort) {
        if (m_serialPort->isOpen()) {
            m_serialPort->close();
        }
        m_serialPort.reset();
        emit connectionStateChanged(false, "Disconnected");
    }
}

void SerialWorker::writeData(const QByteArray &data)
{
    if (m_serialPort && m_serialPort->isOpen()) {
        qint64 written = m_serialPort->write(data);
        if (written != data.size()) {
            emit errorOccurred(QString("Write incomplete: %1/%2 bytes")
                .arg(written).arg(data.size()));
        }
    } else {
        emit errorOccurred("Cannot write: port not open");
    }
}

void SerialWorker::handleReadyRead()
{
    if (!m_serialPort) return;
    
    QByteArray data = m_serialPort->readAll();
    if (!data.isEmpty()) {
        emit rawBytesReady(data);
    }
}

void SerialWorker::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) return;
    
    QString errorMsg;
    switch (error) {
        case QSerialPort::DeviceNotFoundError:
            errorMsg = "Device not found";
            break;
        case QSerialPort::PermissionError:
            errorMsg = "Permission denied";
            break;
        case QSerialPort::OpenError:
            errorMsg = "Failed to open port";
            break;
        case QSerialPort::ReadError:
            errorMsg = "Read error";
            break;
        case QSerialPort::WriteError:
            errorMsg = "Write error";
            break;
        case QSerialPort::ResourceError:
            errorMsg = "Device disconnected";
            closePort();
            break;
        default:
            errorMsg = QString("Error code: %1").arg(static_cast<int>(error));
            break;
    }
    
    emit errorOccurred(errorMsg);
}

// ============================================================================
// SerialManager Implementation
// ============================================================================

SerialManager& SerialManager::instance()
{
    static SerialManager instance;
    return instance;
}

SerialManager::SerialManager()
{
    initWorkerThread();
}

SerialManager::~SerialManager()
{
    // Request port closure before destroying thread
    emit requestClosePort();
    
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(3000);
    }
}

void SerialManager::initWorkerThread()
{
    m_workerThread = std::make_unique<QThread>();
    m_worker = new SerialWorker();  // Will be owned by thread
    m_worker->moveToThread(m_workerThread.get());
    
    // Connect manager signals to worker slots (queued across threads)
    connect(this, &SerialManager::requestOpenPort,
            m_worker, &SerialWorker::openPort);
    connect(this, &SerialManager::requestClosePort,
            m_worker, &SerialWorker::closePort);
    connect(this, &SerialManager::requestWriteData,
            m_worker, &SerialWorker::writeData);
    
    // Connect worker signals back to manager (thread-safe)
    connect(m_worker, &SerialWorker::rawBytesReady,
            this, &SerialManager::rawBytesReady,
            Qt::QueuedConnection);
    connect(m_worker, &SerialWorker::connectionStateChanged,
            this, [this](bool connected, const QString &message) {
                {
                    QMutexLocker locker(&m_mutex);
                    m_isConnected = connected;
                }
                emit connectionStateChanged(connected, message);
            }, Qt::QueuedConnection);
    connect(m_worker, &SerialWorker::errorOccurred,
            this, &SerialManager::errorOccurred,
            Qt::QueuedConnection);
    
    // Clean up worker when thread finishes
    connect(m_workerThread.get(), &QThread::finished,
            m_worker, &QObject::deleteLater);
    
    m_workerThread->start();
}

QList<QSerialPortInfo> SerialManager::availablePorts()
{
    return QSerialPortInfo::availablePorts();
}

bool SerialManager::isConnected() const
{
    QMutexLocker locker(&m_mutex);
    return m_isConnected;
}

SerialSettings SerialManager::currentSettings() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentSettings;
}

void SerialManager::connectPort(const SerialSettings &settings)
{
    {
        QMutexLocker locker(&m_mutex);
        m_currentSettings = settings;
    }
    emit requestOpenPort(settings);
}

void SerialManager::disconnectPort()
{
    emit requestClosePort();
}

void SerialManager::sendData(const QByteArray &data)
{
    emit requestWriteData(data);
}
