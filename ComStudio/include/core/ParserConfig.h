/**
 * @file ParserConfig.h
 * @brief Configuration structure for the universal line parser
 *
 * Defines all configurable options for parsing line-oriented data
 * from serial ports, enabling dynamic field mapping and flexible
 * data extraction.
 */

#ifndef PARSERCONFIG_H
#define PARSERCONFIG_H

#include <QString>
#include <QVector>
#include <QStringList>

/**
 * @enum XAxisSource
 * @brief Defines what to use for X-axis values in plotting
 */
enum class XAxisSource {
    Timestamp,      ///< Use system timestamp (milliseconds)
    Counter,        ///< Use auto-incrementing counter
    FieldIndex      ///< Use a specific field from the data
};

/**
 * @struct ParserConfig
 * @brief Configuration for the LineParser
 *
 * Stores all settings needed to parse line-oriented serial data.
 * Designed for zero-allocation parsing using QStringView where possible.
 */
struct ParserConfig
{
    /**
     * @brief Delimiter character(s) between fields
     *
     * Common values: ",", "\t", " ", ";"
     * Default: ","
     */
    QString delimiter = ",";
    
    /**
     * @brief Index of the sensor/device ID field (-1 to disable)
     *
     * If set, this field is extracted and stored in the packet's sensorId.
     * Useful when multiple sensors share the same serial line.
     */
    int idFieldIndex = -1;
    
    /**
     * @brief Sensor ID to accept (-1 for all)
     *
     * If idFieldIndex is set and this is >= 0, only lines with
     * matching sensor ID will be processed.
     */
    int acceptSensorId = -1;
    
    /**
     * @brief List of field indices to extract as numeric values
     *
     * Specifies which fields (0-indexed) to parse as doubles.
     * Empty list means "extract all numeric fields".
     */
    QVector<int> dataFields;
    
    /**
     * @brief Channel names for each data field
     *
     * Maps to dataFields by index. If shorter than dataFields,
     * remaining channels are named "Ch0", "Ch1", etc.
     */
    QStringList channelNames;
    
    /**
     * @brief Source for X-axis values
     */
    XAxisSource xAxisSource = XAxisSource::Timestamp;
    
    /**
     * @brief Field index to use when xAxisSource is FieldIndex
     */
    int xAxisFieldIndex = 0;
    
    /**
     * @brief Strip non-numeric prefixes from values
     *
     * If true, "X:123.45" becomes "123.45"
     * Useful for labeled data formats
     */
    bool stripLabels = false;
    
    /**
     * @brief Label separator character
     *
     * Character that separates labels from values (e.g., ":" in "X:123")
     */
    QChar labelSeparator = ':';
    
    /**
     * @brief Trim whitespace from tokens
     */
    bool trimWhitespace = true;
    
    /**
     * @brief Skip empty lines
     */
    bool skipEmptyLines = true;
    
    /**
     * @brief Line ending to look for
     *
     * Common values: "\n", "\r\n", "\r"
     */
    QString lineEnding = "\n";
    
    /**
     * @brief Maximum line length before forced flush
     *
     * Prevents buffer overflow on malformed data
     */
    int maxLineLength = 4096;
    
    /**
     * @brief Create default configuration for comma-separated values
     * @return Default CSV configuration
     */
    static ParserConfig csvDefault()
    {
        ParserConfig config;
        config.delimiter = ",";
        config.stripLabels = false;
        return config;
    }
    
    /**
     * @brief Create configuration for tab-separated values
     * @return TSV configuration
     */
    static ParserConfig tsvDefault()
    {
        ParserConfig config;
        config.delimiter = "\t";
        config.stripLabels = false;
        return config;
    }
    
    /**
     * @brief Create configuration for labeled data (e.g., "X:1.0,Y:2.0")
     * @return Labeled data configuration
     */
    static ParserConfig labeledDefault()
    {
        ParserConfig config;
        config.delimiter = ",";
        config.stripLabels = true;
        config.labelSeparator = ':';
        return config;
    }
};

#endif // PARSERCONFIG_H
