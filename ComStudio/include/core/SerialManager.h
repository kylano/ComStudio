/**
 * @file SerialManager.h
 * @brief Serial port communication manager with thread-safe operations
 *
 * Handles all serial port communication in a dedicated worker thread
 * to prevent blocking the UI. Emits signals for data reception and
 * connection state changes.
 */

#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include <QMutex>
#include <QByteArray>
#include <memory>

/**
 * @struct SerialSettings
 * @brief Configuration structure for serial port settings
 */
struct SerialSettings {
    QString portName;
    qint32 baudRate = 115200;
    QSerialPort::DataBits dataBits = QSerialPort::Data8;
    QSerialPort::Parity parity = QSerialPort::NoParity;
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;
};

/**
 * @class SerialWorker
 * @brief Worker class that runs in a separate thread for serial I/O
 *
 * This class performs actual serial port operations in a dedicated
 * thread to ensure non-blocking behavior.
 */
class SerialWorker : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit SerialWorker(QObject *parent = nullptr);
    
    /**
     * @brief Destructor - ensures port is closed
     */
    ~SerialWorker() override;

public slots:
    /**
     * @brief Open serial port with given settings
     * @param settings The serial port configuration
     */
    void openPort(const SerialSettings &settings);
    
    /**
     * @brief Close the serial port
     */
    void closePort();
    
    /**
     * @brief Write data to the serial port
     * @param data Data to write
     */
    void writeData(const QByteArray &data);

signals:
    /**
     * @brief Emitted when raw bytes are received
     * @param data The received data
     */
    void rawBytesReady(const QByteArray &data);
    
    /**
     * @brief Emitted when connection state changes
     * @param connected True if connected, false if disconnected
     * @param message Status message describing the state
     */
    void connectionStateChanged(bool connected, const QString &message);
    
    /**
     * @brief Emitted when an error occurs
     * @param error Error description
     */
    void errorOccurred(const QString &error);

private slots:
    /**
     * @brief Handle incoming data from serial port
     */
    void handleReadyRead();
    
    /**
     * @brief Handle serial port errors
     * @param error The error code
     */
    void handleError(QSerialPort::SerialPortError error);

private:
    std::unique_ptr<QSerialPort> m_serialPort;
    QByteArray m_readBuffer;
};

/**
 * @class SerialManager
 * @brief Main interface for serial communication
 *
 * Provides a thread-safe interface for serial port operations.
 * All actual I/O is performed in a worker thread to keep the UI responsive.
 */
class SerialManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     * @return Reference to the SerialManager instance
     */
    static SerialManager& instance();
    
    /**
     * @brief Destructor
     */
    ~SerialManager() override;
    
    // Delete copy/move constructors for singleton
    SerialManager(const SerialManager&) = delete;
    SerialManager& operator=(const SerialManager&) = delete;
    SerialManager(SerialManager&&) = delete;
    SerialManager& operator=(SerialManager&&) = delete;
    
    /**
     * @brief Get list of available serial ports
     * @return List of available port info objects
     */
    static QList<QSerialPortInfo> availablePorts();
    
    /**
     * @brief Check if currently connected
     * @return True if connected to a serial port
     */
    bool isConnected() const;
    
    /**
     * @brief Get current serial settings
     * @return Current settings
     */
    SerialSettings currentSettings() const;

public slots:
    /**
     * @brief Connect to serial port with given settings
     * @param settings The port configuration
     */
    void connectPort(const SerialSettings &settings);
    
    /**
     * @brief Disconnect from current port
     */
    void disconnectPort();
    
    /**
     * @brief Send data through serial port
     * @param data Data to send
     */
    void sendData(const QByteArray &data);

signals:
    /**
     * @brief Emitted when raw bytes are received from serial port
     * @param data The received raw bytes
     */
    void rawBytesReady(const QByteArray &data);
    
    /**
     * @brief Emitted when connection state changes
     * @param connected True if connected
     * @param message Status message
     */
    void connectionStateChanged(bool connected, const QString &message);
    
    /**
     * @brief Emitted on serial port error
     * @param error Error description
     */
    void errorOccurred(const QString &error);

private:
    /**
     * @brief Private constructor for singleton
     */
    SerialManager();
    
    /**
     * @brief Initialize worker thread
     */
    void initWorkerThread();

    // Internal signals to communicate with worker
signals:
    void requestOpenPort(const SerialSettings &settings);
    void requestClosePort();
    void requestWriteData(const QByteArray &data);

private:
    std::unique_ptr<QThread> m_workerThread;
    SerialWorker *m_worker = nullptr;  // Owned by thread
    
    mutable QMutex m_mutex;
    bool m_isConnected = false;
    SerialSettings m_currentSettings;
};

#endif // SERIALMANAGER_H
