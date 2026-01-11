/**
 * @file BaseProtocol.h
 * @brief Abstract interface for protocol parsers (Strategy Pattern)
 *
 * Defines the contract that all protocol implementations must follow.
 * This enables runtime protocol switching without modifying the
 * serial communication logic.
 */

#ifndef BASEPROTOCOL_H
#define BASEPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <memory>

#include "GenericDataPacket.h"

/**
 * @class BaseProtocol
 * @brief Abstract base class for all protocol parsers
 *
 * Implements the Strategy Pattern, allowing the SerialManager to
 * work with any protocol by holding a pointer to this interface.
 * Concrete implementations handle specific protocols like ASCII,
 * Hex, VOFA-Link, Modbus, etc.
 */
class BaseProtocol : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Virtual destructor for proper cleanup
     */
    ~BaseProtocol() override = default;

    /**
     * @brief Parse incoming raw bytes
     *
     * Process raw data from the serial port and emit dataParsed()
     * for each complete packet/frame that is decoded.
     *
     * @param data Raw bytes received from serial port
     */
    virtual void parse(const QByteArray &data) = 0;
    
    /**
     * @brief Get the protocol name
     * @return Human-readable protocol name
     */
    virtual QString name() const = 0;
    
    /**
     * @brief Get protocol description
     * @return Description of the protocol format
     */
    virtual QString description() const = 0;
    
    /**
     * @brief Reset parser state
     *
     * Clear any internal buffers or state. Called when
     * switching protocols or reconnecting.
     */
    virtual void reset() = 0;
    
    /**
     * @brief Check if protocol supports configuration
     * @return True if protocol has configurable options
     */
    virtual bool isConfigurable() const { return false; }

signals:
    /**
     * @brief Emitted when a complete data packet is parsed
     * @param packet The parsed data in standardized format
     */
    void dataParsed(const GenericDataPacket &packet);
    
    /**
     * @brief Emitted when a parsing error occurs
     * @param error Error description
     * @param rawData The data that caused the error
     */
    void parseError(const QString &error, const QByteArray &rawData);

protected:
    /**
     * @brief Protected constructor (abstract class)
     * @param parent Parent QObject
     */
    explicit BaseProtocol(QObject *parent = nullptr) : QObject(parent) {}
};

/**
 * @brief Shared pointer type for protocol instances
 */
using ProtocolPtr = std::shared_ptr<BaseProtocol>;

#endif // BASEPROTOCOL_H
