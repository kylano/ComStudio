/**
 * @file LineParser.cpp
 * @brief Implementation of LineParser
 */

#include "core/LineParser.h"
#include <QDebug>
#include <charconv>
#include <cstring>

LineParser::LineParser(QObject *parent)
    : BaseProtocol(parent)
    , m_config(ParserConfig::csvDefault())
{
}

LineParser::LineParser(const ParserConfig &config, QObject *parent)
    : BaseProtocol(parent)
    , m_config(config)
{
}

void LineParser::setConfig(const ParserConfig &config)
{
    m_config = config;
    reset();
}

void LineParser::reset()
{
    m_buffer.clear();
    m_packetCounter = 0;
}

void LineParser::parse(const QByteArray &data)
{
    // Append new data to buffer
    m_buffer.append(data);
    
    // Process complete lines
    QByteArray lineEnding = m_config.lineEnding.toUtf8();
    int lineEndLen = lineEnding.length();
    
    int pos = 0;
    while ((pos = m_buffer.indexOf(lineEnding)) != -1) {
        // Check for buffer overflow
        if (pos > m_config.maxLineLength) {
            emit parseError("Line too long, discarding", m_buffer.left(pos));
            m_buffer.remove(0, pos + lineEndLen);
            continue;
        }
        
        // Extract line
        QByteArray lineBytes = m_buffer.left(pos);
        m_buffer.remove(0, pos + lineEndLen);
        
        // Skip empty lines if configured
        if (lineBytes.isEmpty() && m_config.skipEmptyLines) {
            continue;
        }
        
        // Convert to string view for zero-allocation parsing
        QString lineStr = QString::fromUtf8(lineBytes);
        QStringView lineView(lineStr);
        
        if (m_config.trimWhitespace) {
            lineView = lineView.trimmed();
        }
        
        if (lineView.isEmpty() && m_config.skipEmptyLines) {
            continue;
        }
        
        processLine(lineView);
    }
    
    // Check for buffer overflow on incomplete line
    if (m_buffer.size() > m_config.maxLineLength) {
        emit parseError("Buffer overflow, clearing", m_buffer);
        m_buffer.clear();
    }
}

void LineParser::processLine(QStringView line)
{
    // Create packet
    GenericDataPacket packet;
    packet.rawData = line.toUtf8();
    packet.displayText = line.toString();
    packet.packetIndex = m_packetCounter++;
    
    // Split by delimiter
    QStringView delimView(m_config.delimiter);
    QVector<QStringView> tokens = splitLine(line, delimView);
    
    if (tokens.isEmpty()) {
        packet.isValid = false;
        packet.errorMessage = "No tokens found";
        emit parseError(packet.errorMessage, packet.rawData);
        return;
    }
    
    // Check sensor ID if configured
    if (m_config.idFieldIndex >= 0) {
        if (m_config.idFieldIndex < tokens.size()) {
            bool ok = false;
            int id = tokens[m_config.idFieldIndex].toInt(&ok);
            if (ok) {
                packet.sensorId = id;
                
                // Filter by sensor ID if configured
                if (m_config.acceptSensorId >= 0 && id != m_config.acceptSensorId) {
                    return; // Discard packet silently
                }
            }
        }
    }
    
    // Determine which fields to extract
    QVector<int> fieldsToExtract;
    if (m_config.dataFields.isEmpty()) {
        // Extract all fields (except ID field if set)
        for (int i = 0; i < tokens.size(); ++i) {
            if (i != m_config.idFieldIndex) {
                fieldsToExtract.append(i);
            }
        }
    } else {
        fieldsToExtract = m_config.dataFields;
    }
    
    // Extract values
    bool hasError = false;
    for (int i = 0; i < fieldsToExtract.size(); ++i) {
        int fieldIdx = fieldsToExtract[i];
        
        if (fieldIdx < 0 || fieldIdx >= tokens.size()) {
            hasError = true;
            packet.errorMessage = QString("Field index %1 out of range").arg(fieldIdx);
            continue;
        }
        
        QStringView token = tokens[fieldIdx];
        if (m_config.trimWhitespace) {
            token = token.trimmed();
        }
        
        auto maybeValue = extractNumber(token, m_config);
        if (maybeValue.has_value()) {
            // Determine channel name
            QString channelName;
            if (i < m_config.channelNames.size()) {
                channelName = m_config.channelNames[i];
            } else {
                channelName = QString("Ch%1").arg(i);
            }
            
            packet.addChannel(channelName, maybeValue.value());
        } else {
            hasError = true;
            packet.errorMessage = QString("Failed to parse field %1: '%2'")
                .arg(fieldIdx)
                .arg(token.toString());
        }
    }
    
    packet.isValid = packet.hasData() && !hasError;
    
    if (packet.hasData()) {
        emit dataParsed(packet);
    } else if (hasError) {
        emit parseError(packet.errorMessage, packet.rawData);
    }
}

std::optional<double> LineParser::extractNumber(QStringView token, const ParserConfig &config)
{
    if (token.isEmpty()) {
        return std::nullopt;
    }
    
    QStringView numPart = token;
    
    // Strip label prefix if configured
    if (config.stripLabels) {
        int sepPos = token.indexOf(config.labelSeparator);
        if (sepPos >= 0 && sepPos < token.size() - 1) {
            numPart = token.mid(sepPos + 1);
        }
    }
    
    // Trim the number part
    numPart = numPart.trimmed();
    
    if (numPart.isEmpty()) {
        return std::nullopt;
    }
    
    // Use std::from_chars for fast conversion (C++17)
    // Convert to ASCII for from_chars
    QByteArray ascii = numPart.toUtf8();
    double value = 0.0;
    
    auto [ptr, ec] = std::from_chars(ascii.data(), ascii.data() + ascii.size(), value);
    
    if (ec == std::errc()) {
        return value;
    }
    
    // Fallback to Qt conversion for edge cases (e.g., locale-specific formats)
    bool ok = false;
    value = numPart.toDouble(&ok);
    if (ok) {
        return value;
    }
    
    return std::nullopt;
}

QVector<QStringView> LineParser::splitLine(QStringView line, QStringView delimiter)
{
    QVector<QStringView> tokens;
    
    if (line.isEmpty()) {
        return tokens;
    }
    
    if (delimiter.isEmpty()) {
        tokens.append(line);
        return tokens;
    }
    
    int start = 0;
    int pos = 0;
    
    while ((pos = line.indexOf(delimiter, start)) != -1) {
        tokens.append(line.mid(start, pos - start));
        start = pos + delimiter.length();
    }
    
    // Add remaining part
    tokens.append(line.mid(start));
    
    return tokens;
}

ParseResult LineParser::testParse(const QString &sampleLine, const ParserConfig &config)
{
    ParseResult result;
    result.originalLine = sampleLine;
    
    QStringView lineView(sampleLine);
    
    if (config.trimWhitespace) {
        lineView = lineView.trimmed();
    }
    
    if (lineView.isEmpty()) {
        result.success = false;
        result.errorMessage = "Empty line";
        return result;
    }
    
    // Split by delimiter
    QStringView delimView(config.delimiter);
    QVector<QStringView> tokens = splitLine(lineView, delimView);
    
    if (tokens.isEmpty()) {
        result.success = false;
        result.errorMessage = "No tokens found";
        return result;
    }
    
    // Store field texts
    for (const auto &token : tokens) {
        result.fieldTexts.append(token.toString());
    }
    
    // Determine which fields to extract
    QVector<int> fieldsToExtract;
    if (config.dataFields.isEmpty()) {
        for (int i = 0; i < tokens.size(); ++i) {
            if (i != config.idFieldIndex) {
                fieldsToExtract.append(i);
            }
        }
    } else {
        fieldsToExtract = config.dataFields;
    }
    
    // Try to extract values
    result.success = true;
    for (int i = 0; i < fieldsToExtract.size(); ++i) {
        int fieldIdx = fieldsToExtract[i];
        
        if (fieldIdx < 0 || fieldIdx >= tokens.size()) {
            result.success = false;
            result.errorMessage = QString("Field index %1 out of range (have %2 fields)")
                .arg(fieldIdx).arg(tokens.size());
            result.failedFieldIndex = fieldIdx;
            continue;
        }
        
        QStringView token = tokens[fieldIdx];
        if (config.trimWhitespace) {
            token = token.trimmed();
        }
        
        auto maybeValue = extractNumber(token, config);
        if (maybeValue.has_value()) {
            result.values.append(maybeValue.value());
        } else {
            result.success = false;
            result.errorMessage = QString("Failed to parse field %1: '%2'")
                .arg(fieldIdx).arg(token.toString());
            result.failedFieldIndex = fieldIdx;
        }
    }
    
    if (result.success && result.values.isEmpty()) {
        result.success = false;
        result.errorMessage = "No numeric values extracted";
    }
    
    return result;
}

ParseResult LineParser::testParse(const QString &sampleLine) const
{
    return testParse(sampleLine, m_config);
}
