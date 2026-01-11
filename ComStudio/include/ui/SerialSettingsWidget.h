/**
 * @file SerialSettingsWidget.h
 * @brief Widget for configuring serial port settings
 *
 * Provides UI controls for selecting port, baud rate, data bits,
 * parity, stop bits, and flow control. Also handles connect/disconnect.
 */

#ifndef SERIALSETTINGSWIDGET_H
#define SERIALSETTINGSWIDGET_H

#include <QWidget>
#include <QSerialPort>

#include "core/SerialManager.h"

// Forward declarations
class QComboBox;
class QPushButton;
class QLabel;

/**
 * @class SerialSettingsWidget
 * @brief UI widget for serial port configuration
 *
 * Provides drop-down selectors for all serial port parameters
 * and connect/disconnect buttons.
 */
class SerialSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit SerialSettingsWidget(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~SerialSettingsWidget() override = default;
    
    /**
     * @brief Get current settings from UI
     * @return SerialSettings structure with current values
     */
    SerialSettings currentSettings() const;
    
    /**
     * @brief Refresh the list of available ports
     */
    void refreshPorts();

public slots:
    /**
     * @brief Handle connection state changes
     * @param connected True if connected
     * @param message Status message
     */
    void onConnectionStateChanged(bool connected, const QString &message);

signals:
    /**
     * @brief Emitted when user clicks Connect
     * @param settings The selected serial settings
     */
    void connectRequested(const SerialSettings &settings);
    
    /**
     * @brief Emitted when user clicks Disconnect
     */
    void disconnectRequested();

private slots:
    /**
     * @brief Handle Connect button click
     */
    void onConnectClicked();
    
    /**
     * @brief Handle Disconnect button click
     */
    void onDisconnectClicked();
    
    /**
     * @brief Handle Refresh button click
     */
    void onRefreshClicked();

private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Populate combo boxes with options
     */
    void populateOptions();
    
    /**
     * @brief Update UI based on connection state
     * @param connected True if connected
     */
    void updateConnectionState(bool connected);

    // UI Elements
    QComboBox *m_portCombo = nullptr;
    QComboBox *m_baudRateCombo = nullptr;
    QComboBox *m_dataBitsCombo = nullptr;
    QComboBox *m_parityCombo = nullptr;
    QComboBox *m_stopBitsCombo = nullptr;
    QComboBox *m_flowControlCombo = nullptr;
    
    QPushButton *m_connectButton = nullptr;
    QPushButton *m_disconnectButton = nullptr;
    QPushButton *m_refreshButton = nullptr;
    
    QLabel *m_statusLabel = nullptr;
    
    bool m_isConnected = false;
};

#endif // SERIALSETTINGSWIDGET_H
