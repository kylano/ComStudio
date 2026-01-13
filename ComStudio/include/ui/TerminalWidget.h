/**
 * @file TerminalWidget.h
 * @brief Terminal view for displaying serial data
 *
 * Provides a scrollable text view for displaying incoming
 * serial data in various formats (raw, hex, parsed).
 * Optimized with batched updates for high-throughput data.
 */

#ifndef TERMINALWIDGET_H
#define TERMINALWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QTimer>
#include <QElapsedTimer>

#include "core/GenericDataPacket.h"

/**
 * @enum DisplayMode
 * @brief Terminal display format options
 */
enum class DisplayMode {
    Raw,        ///< Raw text as received
    Hex,        ///< Hexadecimal display
    Parsed      ///< Show parsed values
};

/**
 * @enum LineEnding
 * @brief Line ending options for sending data
 */
enum class LineEnding {
    None,       ///< No line ending
    LF,         ///< \n (Unix)
    CRLF        ///< \r\n (Windows)
};

/**
 * @enum SendMode
 * @brief Send mode options
 */
enum class SendMode {
    ASCII,      ///< Send as ASCII text
    Hex         ///< Send as hex bytes
};

/**
 * @class TerminalWidget
 * @brief Widget for terminal-style data display
 *
 * Displays incoming serial data with options for different
 * display formats and filtering.
 */
class TerminalWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit TerminalWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~TerminalWidget() override = default;
    
    /**
     * @brief Get current display mode
     * @return Current display mode
     */
    DisplayMode displayMode() const { return m_displayMode; }
    
    /**
     * @brief Set display mode
     * @param mode New display mode
     */
    void setDisplayMode(DisplayMode mode);
    
    /**
     * @brief Set maximum number of lines to display
     * @param maxLines Maximum lines (0 = unlimited)
     */
    void setMaxLines(int maxLines);

public slots:
    /**
     * @brief Append raw bytes to terminal (optimized with batching)
     * @param data Raw bytes
     */
    void appendRawData(const QByteArray &data);
    
    /**
     * @brief Append raw line to terminal (for LineParser rawLineReady signal)
     * @param line Raw line string
     */
    void appendRawLine(const QString &line);
    
    /**
     * @brief Append parsed data packet (batched for performance)
     * @param packet Parsed packet
     */
    void appendPacket(const GenericDataPacket &packet);
    
    /**
     * @brief Clear terminal content
     */
    void clear();
    
    /**
     * @brief Flush any pending batched data immediately
     */
    void flushPendingData();

signals:
    /**
     * @brief Emitted when user sends data
     * @param data Data to send
     */
    void sendData(const QByteArray &data);

private slots:
    /**
     * @brief Handle display mode change
     * @param index Combo box index
     */
    void onDisplayModeChanged(int index);
    
    /**
     * @brief Handle send button click
     */
    void onSendClicked();
    
    /**
     * @brief Handle auto-scroll toggle
     * @param enabled Auto-scroll state
     */
    void onAutoScrollToggled(bool enabled);
    
    /**
     * @brief Flush batched updates to terminal (called by timer)
     */
    void onFlushTimer();

private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Format data according to display mode
     * @param data Raw data
     * @return Formatted string
     */
    QString formatData(const QByteArray &data) const;
    
    /**
     * @brief Format packet for display
     * @param packet Parsed packet
     * @return Formatted string
     */
    QString formatPacket(const GenericDataPacket &packet) const;
    
    /**
     * @brief Trim excess lines from terminal
     */
    void trimLines();

    QPlainTextEdit *m_terminal = nullptr;
    QComboBox *m_displayModeCombo = nullptr;
    QLineEdit *m_sendInput = nullptr;
    QPushButton *m_sendButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QCheckBox *m_autoScrollCheck = nullptr;
    QCheckBox *m_timestampCheck = nullptr;
    
    // Send controls
    QComboBox *m_lineEndingCombo = nullptr;
    QComboBox *m_sendModeCombo = nullptr;
    
    DisplayMode m_displayMode = DisplayMode::Raw;
    LineEnding m_lineEnding = LineEnding::LF;
    SendMode m_sendMode = SendMode::ASCII;
    int m_maxLines = 10000;
    bool m_autoScroll = true;
    
    // Batched update optimization
    QTimer *m_flushTimer = nullptr;
    QString m_pendingRawText;          ///< Buffered raw text
    QVector<GenericDataPacket> m_pendingPackets;  ///< Buffered packets for parsed mode
    static constexpr int FLUSH_INTERVAL_MS = 50;  ///< Batch flush interval (~20 FPS)
    static constexpr int MAX_PENDING_CHARS = 8192; ///< Max chars before forced flush
    static constexpr int MAX_PENDING_PACKETS = 100; ///< Max packets before forced flush
};

#endif // TERMINALWIDGET_H
