/**
 * @file DataBuffer.cpp
 * @brief Implementation of DataBuffer
 */

#include "models/DataBuffer.h"
#include <QDebug>

DataBuffer::DataBuffer(int maxSize, QObject *parent)
    : QObject(parent)
    , m_maxSize(maxSize)
{
}

void DataBuffer::setMaxSize(int size)
{
    QWriteLocker locker(&m_lock);
    m_maxSize = size;
    
    // Trim if necessary
    while (static_cast<int>(m_packets.size()) > m_maxSize) {
        m_packets.pop_front();
    }
}

int DataBuffer::size() const
{
    QReadLocker locker(&m_lock);
    return static_cast<int>(m_packets.size());
}

bool DataBuffer::isEmpty() const
{
    QReadLocker locker(&m_lock);
    return m_packets.empty();
}

GenericDataPacket DataBuffer::packetAt(int index) const
{
    QReadLocker locker(&m_lock);
    if (index >= 0 && index < static_cast<int>(m_packets.size())) {
        return m_packets[index];
    }
    return GenericDataPacket();
}

GenericDataPacket DataBuffer::lastPacket() const
{
    QReadLocker locker(&m_lock);
    if (!m_packets.empty()) {
        return m_packets.back();
    }
    return GenericDataPacket();
}

QVector<GenericDataPacket> DataBuffer::allPackets() const
{
    QReadLocker locker(&m_lock);
    QVector<GenericDataPacket> result;
    result.reserve(static_cast<int>(m_packets.size()));
    for (const auto &packet : m_packets) {
        result.append(packet);
    }
    return result;
}

QVector<GenericDataPacket> DataBuffer::packets(int start, int count) const
{
    QReadLocker locker(&m_lock);
    QVector<GenericDataPacket> result;
    
    int packetCount = static_cast<int>(m_packets.size());
    if (start < 0 || start >= packetCount) {
        return result;
    }
    
    int end = qMin(start + count, packetCount);
    result.reserve(end - start);
    
    for (int i = start; i < end; ++i) {
        result.append(m_packets[i]);
    }
    
    return result;
}

void DataBuffer::channelData(const QString &channelName,
                             QVector<double> &timestamps,
                             QVector<double> &values,
                             int maxPoints) const
{
    QReadLocker locker(&m_lock);
    
    timestamps.clear();
    values.clear();
    
    int count = static_cast<int>(m_packets.size());
    int start = 0;
    if (maxPoints > 0 && count > maxPoints) {
        start = count - maxPoints;
    }
    
    timestamps.reserve(count - start);
    values.reserve(count - start);
    
    for (int i = start; i < count; ++i) {
        const auto &packet = m_packets[i];
        if (packet.channels.contains(channelName)) {
            timestamps.append(static_cast<double>(packet.timestamp));
            values.append(packet.channels.value(channelName));
        }
    }
}

void DataBuffer::channelDataByIndex(int channelIndex,
                                    QVector<double> &timestamps,
                                    QVector<double> &values,
                                    int maxPoints) const
{
    QReadLocker locker(&m_lock);
    
    timestamps.clear();
    values.clear();
    
    int count = static_cast<int>(m_packets.size());
    int start = 0;
    if (maxPoints > 0 && count > maxPoints) {
        start = count - maxPoints;
    }
    
    timestamps.reserve(count - start);
    values.reserve(count - start);
    
    for (int i = start; i < count; ++i) {
        const auto &packet = m_packets[i];
        if (channelIndex >= 0 && channelIndex < packet.values.size()) {
            timestamps.append(static_cast<double>(packet.timestamp));
            values.append(packet.values[channelIndex]);
        }
    }
}

QStringList DataBuffer::channelNames() const
{
    QReadLocker locker(&m_lock);
    return m_channelNames;
}

int DataBuffer::maxChannelCount() const
{
    QReadLocker locker(&m_lock);
    return m_maxChannelCount;
}

void DataBuffer::addPacket(const GenericDataPacket &packet)
{
    {
        QWriteLocker locker(&m_lock);
        
        // Add packet to buffer
        m_packets.push_back(packet);
        
        // Trim if over capacity
        while (static_cast<int>(m_packets.size()) > m_maxSize) {
            m_packets.pop_front();
        }
        
        // Track channel names
        bool newChannels = false;
        for (auto it = packet.channels.constBegin(); it != packet.channels.constEnd(); ++it) {
            if (!m_channelNames.contains(it.key())) {
                m_channelNames.append(it.key());
                newChannels = true;
            }
        }
        
        // Track max channel count
        if (packet.channelCount() > m_maxChannelCount) {
            m_maxChannelCount = packet.channelCount();
        }
        
        // Emit channel added signals outside lock scope
        if (newChannels) {
            for (auto it = packet.channels.constBegin(); it != packet.channels.constEnd(); ++it) {
                if (!m_channelNames.contains(it.key())) {
                    // This won't be reached due to logic above, but keeping for clarity
                }
            }
        }
    }
    
    // Emit signals outside lock
    emit dataUpdated(packet);
    
    // Check for new channels and emit signals
    for (auto it = packet.channels.constBegin(); it != packet.channels.constEnd(); ++it) {
        QReadLocker locker(&m_lock);
        // Find if this was a newly added channel
        int idx = m_channelNames.indexOf(it.key());
        if (idx == m_channelNames.size() - 1) {
            locker.unlock();
            emit channelAdded(it.key());
        }
    }
}

void DataBuffer::clear()
{
    {
        QWriteLocker locker(&m_lock);
        m_packets.clear();
        m_channelNames.clear();
        m_maxChannelCount = 0;
    }
    
    emit cleared();
}
