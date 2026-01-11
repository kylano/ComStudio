/**
 * @file ProtocolHandler.h
 * @brief Protocol routing and management
 *
 * Acts as the central coordinator between SerialManager and
 * the active protocol parser. Implements strategy pattern to
 * allow runtime protocol switching.
 */

#ifndef PROTOCOLHANDLER_H
#define PROTOCOLHANDLER_H

#include <QObject>
#include <QByteArray>
#include <QMap>
#include <QString>
#include <memory>

#include "BaseProtocol.h"
#include "GenericDataPacket.h"

/**
 * @class ProtocolHandler
 * @brief Routes data to the active protocol parser
 *
 * Manages protocol registration and switching. Receives raw bytes
 * from SerialManager and forwards them to the currently active
 * protocol parser.
 */
class ProtocolHandler : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    explicit ProtocolHandler(QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~ProtocolHandler() override = default;
    
    /**
     * @brief Register a protocol parser
     * @param id Unique identifier for the protocol
     * @param protocol Shared pointer to the protocol instance
     */
    void registerProtocol(const QString &id, ProtocolPtr protocol);
    
    /**
     * @brief Unregister a protocol parser
     * @param id Protocol identifier to remove
     */
    void unregisterProtocol(const QString &id);
    
    /**
     * @brief Set the active protocol
     * @param id Protocol identifier to activate
     * @return True if protocol was found and activated
     */
    bool setActiveProtocol(const QString &id);
    
    /**
     * @brief Get the currently active protocol ID
     * @return Active protocol identifier
     */
    QString activeProtocolId() const;
    
    /**
     * @brief Get the active protocol instance
     * @return Pointer to active protocol, or nullptr
     */
    BaseProtocol* activeProtocol() const;
    
    /**
     * @brief Get list of registered protocol IDs
     * @return List of protocol identifiers
     */
    QStringList registeredProtocols() const;
    
    /**
     * @brief Get protocol by ID
     * @param id Protocol identifier
     * @return Protocol pointer or nullptr
     */
    BaseProtocol* protocol(const QString &id) const;
    
    /**
     * @brief Reset the active protocol's state
     */
    void resetParser();

public slots:
    /**
     * @brief Process raw bytes from serial port
     *
     * Forwards data to the active protocol parser.
     * Should be connected to SerialManager::rawBytesReady.
     *
     * @param data Raw bytes to process
     */
    void processRawData(const QByteArray &data);

signals:
    /**
     * @brief Emitted when parsed data is ready
     *
     * Re-emits the active protocol's dataParsed signal.
     * UI components should connect to this signal.
     *
     * @param packet Parsed data packet
     */
    void dataParsed(const GenericDataPacket &packet);
    
    /**
     * @brief Emitted when a parsing error occurs
     * @param error Error description
     * @param rawData The problematic data
     */
    void parseError(const QString &error, const QByteArray &rawData);
    
    /**
     * @brief Emitted when the active protocol changes
     * @param protocolId New active protocol ID
     */
    void protocolChanged(const QString &protocolId);

private:
    /**
     * @brief Connect signals from a protocol
     * @param protocol Protocol to connect
     */
    void connectProtocolSignals(BaseProtocol *protocol);
    
    /**
     * @brief Disconnect signals from a protocol
     * @param protocol Protocol to disconnect
     */
    void disconnectProtocolSignals(BaseProtocol *protocol);

    QMap<QString, ProtocolPtr> m_protocols;
    QString m_activeProtocolId;
};

#endif // PROTOCOLHANDLER_H
