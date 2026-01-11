/**
 * @file GenericDataPacket.h
 * @brief Standardized data packet structure for protocol output
 *
 * GenericDataPacket provides a protocol-agnostic data container
 * that all protocol parsers emit. This allows UI components to
 * receive data in a consistent format regardless of the underlying
 * protocol (ASCII, Hex, VOFA, Modbus, etc.).
 */

#ifndef GENERICDATAPACKET_H
#define GENERICDATAPACKET_H

#include <QMap>
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QByteArray>
#include <cstdint>

/**
 * @struct GenericDataPacket
 * @brief Universal data packet emitted by all protocol parsers
 *
 * Contains parsed channel data along with metadata like timestamps
 * and the original raw data for debugging/logging purposes.
 */
struct GenericDataPacket
{
    /**
     * @brief Timestamp when the data was received/parsed
     */
    qint64 timestamp = 0;
    
    /**
     * @brief Packet counter (auto-incrementing)
     */
    quint64 packetIndex = 0;
    
    /**
     * @brief Sensor/device ID (if applicable, -1 if not used)
     */
    int sensorId = -1;
    
    /**
     * @brief Channel name to value mapping
     *
     * Keys are channel identifiers (e.g., "X", "Y", "Z", "Ch0", etc.)
     * Values are the parsed double values
     */
    QMap<QString, double> channels;
    
    /**
     * @brief Ordered list of values (for indexed access)
     *
     * Same values as in channels map but in the order they were parsed.
     * Useful for plotting when channel names aren't important.
     */
    QVector<double> values;
    
    /**
     * @brief Original raw data that produced this packet
     *
     * Stored for debugging, logging, or raw terminal display
     */
    QByteArray rawData;
    
    /**
     * @brief Human-readable formatted string
     *
     * Pre-formatted string for terminal display
     */
    QString displayText;
    
    /**
     * @brief Indicates if parsing was successful
     */
    bool isValid = false;
    
    /**
     * @brief Error message if parsing failed
     */
    QString errorMessage;
    
    /**
     * @brief Default constructor
     */
    GenericDataPacket() : timestamp(QDateTime::currentMSecsSinceEpoch()) {}
    
    /**
     * @brief Constructor with raw data
     * @param raw The raw bytes that will be parsed
     */
    explicit GenericDataPacket(const QByteArray &raw)
        : timestamp(QDateTime::currentMSecsSinceEpoch())
        , rawData(raw)
    {}
    
    /**
     * @brief Add a channel value
     * @param name Channel name
     * @param value Channel value
     */
    void addChannel(const QString &name, double value)
    {
        channels[name] = value;
        values.append(value);
    }
    
    /**
     * @brief Get value by channel name
     * @param name Channel name
     * @param defaultValue Value to return if channel not found
     * @return Channel value or default
     */
    double value(const QString &name, double defaultValue = 0.0) const
    {
        return channels.value(name, defaultValue);
    }
    
    /**
     * @brief Get value by index
     * @param index Value index
     * @param defaultValue Value to return if index out of range
     * @return Value at index or default
     */
    double valueAt(int index, double defaultValue = 0.0) const
    {
        if (index >= 0 && index < values.size()) {
            return values.at(index);
        }
        return defaultValue;
    }
    
    /**
     * @brief Get number of channels/values
     * @return Number of parsed values
     */
    int channelCount() const
    {
        return values.size();
    }
    
    /**
     * @brief Check if packet has any data
     * @return True if at least one channel has data
     */
    bool hasData() const
    {
        return !values.isEmpty();
    }
};

/**
 * @struct MultiChannelData
 * @brief Alias for backward compatibility and semantic clarity
 */
using MultiChannelData = GenericDataPacket;

#endif // GENERICDATAPACKET_H
