/**
 * @file LineParser.h
 * @brief High-performance line-oriented data parser
 *
 * Zero-allocation parser using QStringView for maximum performance
 * at high baud rates. Supports configurable delimiters, field mapping,
 * and label stripping.
 */

#ifndef LINEPARSER_H
#define LINEPARSER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringView>
#include <QVector>
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
};

#endif // LINEPARSER_H
