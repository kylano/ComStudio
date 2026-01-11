/**
 * @file DataBuffer.h
 * @brief Central data storage for parsed serial data
 *
 * Provides a thread-safe ring buffer for storing parsed data packets.
 * Both TerminalWidget and PlotterWidget subscribe to this buffer
 * for updates.
 */

#ifndef DATABUFFER_H
#define DATABUFFER_H

#include <QObject>
#include <QVector>
#include <QMutex>
#include <QReadWriteLock>
#include <deque>

#include "core/GenericDataPacket.h"

/**
 * @class DataBuffer
 * @brief Ring buffer for storing parsed data packets
 *
 * Stores a configurable number of recent packets in a ring buffer.
 * Provides both raw packet access and per-channel time-series data
 * for plotting.
 */
class DataBuffer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param maxSize Maximum number of packets to store
     * @param parent Parent QObject
     */
    explicit DataBuffer(int maxSize = 10000, QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~DataBuffer() override = default;
    
    /**
     * @brief Get maximum buffer size
     * @return Maximum number of packets stored
     */
    int maxSize() const { return m_maxSize; }
    
    /**
     * @brief Set maximum buffer size
     * @param size New maximum size
     */
    void setMaxSize(int size);
    
    /**
     * @brief Get current number of stored packets
     * @return Number of packets in buffer
     */
    int size() const;
    
    /**
     * @brief Check if buffer is empty
     * @return True if no packets stored
     */
    bool isEmpty() const;
    
    /**
     * @brief Get packet at index
     * @param index Packet index (0 = oldest)
     * @return Packet at index, or empty packet if out of range
     */
    GenericDataPacket packetAt(int index) const;
    
    /**
     * @brief Get most recent packet
     * @return Latest packet or empty packet if buffer empty
     */
    GenericDataPacket lastPacket() const;
    
    /**
     * @brief Get all packets (thread-safe copy)
     * @return Vector of all stored packets
     */
    QVector<GenericDataPacket> allPackets() const;
    
    /**
     * @brief Get packets in range
     * @param start Start index
     * @param count Number of packets
     * @return Vector of packets in range
     */
    QVector<GenericDataPacket> packets(int start, int count) const;
    
    /**
     * @brief Get time-series data for a channel
     *
     * Returns parallel vectors of timestamps and values for plotting.
     *
     * @param channelName Name of the channel
     * @param timestamps Output vector for timestamps
     * @param values Output vector for values
     * @param maxPoints Maximum number of points to return (0 = all)
     */
    void channelData(const QString &channelName,
                     QVector<double> &timestamps,
                     QVector<double> &values,
                     int maxPoints = 0) const;
    
    /**
     * @brief Get time-series data by channel index
     * @param channelIndex Index of the channel (0-based)
     * @param timestamps Output vector for timestamps
     * @param values Output vector for values
     * @param maxPoints Maximum number of points to return
     */
    void channelDataByIndex(int channelIndex,
                            QVector<double> &timestamps,
                            QVector<double> &values,
                            int maxPoints = 0) const;
    
    /**
     * @brief Get list of known channel names
     * @return List of channel names seen in data
     */
    QStringList channelNames() const;
    
    /**
     * @brief Get maximum number of channels seen
     * @return Maximum channel count
     */
    int maxChannelCount() const;

public slots:
    /**
     * @brief Add a new packet to the buffer
     * @param packet Packet to add
     */
    void addPacket(const GenericDataPacket &packet);
    
    /**
     * @brief Clear all stored data
     */
    void clear();

signals:
    /**
     * @brief Emitted when new data is added
     * @param packet The newly added packet
     */
    void dataUpdated(const GenericDataPacket &packet);
    
    /**
     * @brief Emitted when buffer is cleared
     */
    void cleared();
    
    /**
     * @brief Emitted when a new channel is discovered
     * @param channelName Name of the new channel
     */
    void channelAdded(const QString &channelName);

private:
    mutable QReadWriteLock m_lock;
    std::deque<GenericDataPacket> m_packets;
    int m_maxSize;
    QStringList m_channelNames;
    int m_maxChannelCount = 0;
};

#endif // DATABUFFER_H
