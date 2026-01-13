/**
 * @file LineParser.h
 * @brief High-performance line-oriented data parser
 *
 * Zero-allocation parser using QStringView for maximum performance
 * at high baud rates. Supports configurable delimiters, field mapping,
 * label stripping, and rate-limited output for UI performance.
 */

#ifndef LINEPARSER_H
#define LINEPARSER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringView>
#include <QVector>
#include <QElapsedTimer>
#include <optional>

#include "BaseProtocol.h"
#include "ParserConfig.h"
#include "GenericDataPacket.h"

/**
 * @struct ParseResult
 * @brief Detailed result from test parsing
 */
struct ParseResult
{
    bool success = false;
    QString errorMessage;
    int failedFieldIndex = -1;
    QVector<double> values;
    QStringList fieldTexts;
    QString originalLine;
};

/**
 * @class LineParser
 * @brief High-performance line-oriented data parser
 *
 * Implements the BaseProtocol interface for line-based data formats.
 * Uses QStringView and zero-allocation techniques for performance
 * at high data rates.
 */
class LineParser : public BaseProtocol
{
    Q_OBJECT

public:
    /**
     * @brief Constructor with default configuration
     * @param parent Parent QObject
     */
    explicit LineParser(QObject *parent = nullptr);
    
    /**
     * @brief Constructor with custom configuration
     * @param config Parser configuration
     * @param parent Parent QObject
     */
    explicit LineParser(const ParserConfig &config, QObject *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~LineParser() override = default;
    
    // BaseProtocol interface
    void parse(const QByteArray &data) override;
    QString name() const override { return QStringLiteral("Line Parser"); }
    QString description() const override { return QStringLiteral("Universal line-oriented data parser"); }
    void reset() override;
    bool isConfigurable() const override { return true; }
    
    /**
     * @brief Get current configuration
     * @return Current parser configuration
     */
    ParserConfig config() const { return m_config; }
    
    /**
     * @brief Set parser configuration
     * @param config New configuration
     */
    void setConfig(const ParserConfig &config);
    
    /**
     * @brief Set target display rate for rate-limited output
     * @param hz Target rate in Hz (0 = no limit, emit all packets)
     */
    void setTargetDisplayRate(int hz);
    
    /**
     * @brief Get current target display rate
     * @return Current rate in Hz
     */
    int targetDisplayRate() const { return m_targetDisplayRate; }
    
    /**
     * @brief Enable/disable rate limiting
     * @param enabled True to enable rate limiting
     */
    void setRateLimitEnabled(bool enabled) { m_rateLimitEnabled = enabled; }
    
    /**
     * @brief Check if rate limiting is enabled
     * @return True if rate limiting is active
     */
    bool isRateLimitEnabled() const { return m_rateLimitEnabled; }
    
    /**
     * @brief Test parse a sample line
     *
     * Static method for testing configuration against sample data
     * without affecting parser state.
     *
     * @param sampleLine Line to test parse
     * @param config Configuration to use
     * @return Detailed parse result
     */
    static ParseResult testParse(const QString &sampleLine, const ParserConfig &config);
    
    /**
     * @brief Test parse using current configuration
     * @param sampleLine Line to test parse
     * @return Detailed parse result
     */
    ParseResult testParse(const QString &sampleLine) const;

signals:
    /**
     * @brief Emitted for every raw line received (no rate limiting)
     * 
     * Use this for terminal raw mode display. Always emitted regardless
     * of rate limiting settings.
     * 
     * @param line The raw line as received
     */
    void rawLineReady(const QString &line);
    
    /**
     * @brief Emitted for every valid packet (no rate limiting)
     * 
     * Use this for data logging where all samples must be recorded.
     * Bypasses display rate limiting.
     * 
     * @param packet The parsed packet
     */
    void dataForLogging(const GenericDataPacket &packet);

private:
    /**
     * @brief Process a complete line
     * @param line The line to process (without line ending)
     */
    void processLine(QStringView line);
    
    /**
     * @brief Extract numeric value from a token
     *
     * Handles label stripping and conversion to double.
     *
     * @param token The token to parse
     * @param config Configuration for label handling
     * @return Parsed value or std::nullopt on failure
     */
    static std::optional<double> extractNumber(QStringView token, const ParserConfig &config);
    
    /**
     * @brief Split line into tokens using delimiter
     * @param line Line to split
     * @param delimiter Delimiter string
     * @return Vector of token views (zero-copy)
     */
    static QVector<QStringView> splitLine(QStringView line, QStringView delimiter);

    ParserConfig m_config;
    QByteArray m_buffer;           ///< Accumulation buffer for incomplete lines
    quint64 m_packetCounter = 0;   ///< Auto-incrementing packet counter
    
    // Rate limiting for display performance
    QElapsedTimer m_elapsedTimer;  ///< High-resolution timer for rate limiting
    qint64 m_lastEmitTimestamp = 0; ///< Last time dataParsed was emitted
    double m_targetIntervalMs = 16.67; ///< Target interval between emissions (ms)
    int m_targetDisplayRate = 60;  ///< Target display rate in Hz
    bool m_rateLimitEnabled = true; ///< Whether rate limiting is active
    bool m_timerStarted = false;   ///< Whether elapsed timer has been started
};

#endif // LINEPARSER_H
