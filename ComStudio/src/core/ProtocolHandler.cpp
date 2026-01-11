/**
 * @file ProtocolHandler.cpp
 * @brief Implementation of ProtocolHandler
 */

#include "core/ProtocolHandler.h"
#include <QDebug>

ProtocolHandler::ProtocolHandler(QObject *parent)
    : QObject(parent)
{
}

void ProtocolHandler::registerProtocol(const QString &id, ProtocolPtr protocol)
{
    if (!protocol) {
        qWarning() << "ProtocolHandler: Cannot register null protocol";
        return;
    }
    
    if (m_protocols.contains(id)) {
        qWarning() << "ProtocolHandler: Protocol" << id << "already registered, replacing";
        unregisterProtocol(id);
    }
    
    m_protocols[id] = protocol;
    qDebug() << "ProtocolHandler: Registered protocol" << id;
    
    // If this is the first protocol, make it active
    if (m_activeProtocolId.isEmpty()) {
        setActiveProtocol(id);
    }
}

void ProtocolHandler::unregisterProtocol(const QString &id)
{
    if (!m_protocols.contains(id)) {
        return;
    }
    
    // If removing active protocol, disconnect signals
    if (id == m_activeProtocolId) {
        disconnectProtocolSignals(m_protocols[id].get());
        m_activeProtocolId.clear();
    }
    
    m_protocols.remove(id);
}

bool ProtocolHandler::setActiveProtocol(const QString &id)
{
    if (!m_protocols.contains(id)) {
        qWarning() << "ProtocolHandler: Unknown protocol" << id;
        return false;
    }
    
    // Disconnect from previous protocol
    if (!m_activeProtocolId.isEmpty() && m_protocols.contains(m_activeProtocolId)) {
        disconnectProtocolSignals(m_protocols[m_activeProtocolId].get());
    }
    
    // Connect to new protocol
    m_activeProtocolId = id;
    connectProtocolSignals(m_protocols[id].get());
    
    // Reset parser state for clean start
    m_protocols[id]->reset();
    
    emit protocolChanged(id);
    qDebug() << "ProtocolHandler: Active protocol set to" << id;
    
    return true;
}

QString ProtocolHandler::activeProtocolId() const
{
    return m_activeProtocolId;
}

BaseProtocol* ProtocolHandler::activeProtocol() const
{
    if (m_activeProtocolId.isEmpty() || !m_protocols.contains(m_activeProtocolId)) {
        return nullptr;
    }
    return m_protocols[m_activeProtocolId].get();
}

QStringList ProtocolHandler::registeredProtocols() const
{
    return m_protocols.keys();
}

BaseProtocol* ProtocolHandler::protocol(const QString &id) const
{
    if (!m_protocols.contains(id)) {
        return nullptr;
    }
    return m_protocols[id].get();
}

void ProtocolHandler::resetParser()
{
    if (auto *proto = activeProtocol()) {
        proto->reset();
    }
}

void ProtocolHandler::processRawData(const QByteArray &data)
{
    if (auto *proto = activeProtocol()) {
        proto->parse(data);
    }
}

void ProtocolHandler::connectProtocolSignals(BaseProtocol *protocol)
{
    if (!protocol) return;
    
    connect(protocol, &BaseProtocol::dataParsed,
            this, &ProtocolHandler::dataParsed);
    connect(protocol, &BaseProtocol::parseError,
            this, &ProtocolHandler::parseError);
}

void ProtocolHandler::disconnectProtocolSignals(BaseProtocol *protocol)
{
    if (!protocol) return;
    
    disconnect(protocol, &BaseProtocol::dataParsed,
               this, &ProtocolHandler::dataParsed);
    disconnect(protocol, &BaseProtocol::parseError,
               this, &ProtocolHandler::parseError);
}
