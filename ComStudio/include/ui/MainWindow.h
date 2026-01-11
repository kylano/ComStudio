/**
 * @file MainWindow.h
 * @brief Main application window
 *
 * The main window that contains all UI components including
 * serial settings, terminal view, and plotter.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

#include "core/SerialManager.h"
#include "core/GenericDataPacket.h"

// Forward declarations
class SerialSettingsWidget;
class TerminalWidget;
class PlotterWidget;
class ProtocolHandler;
class DataBuffer;
class QTabWidget;
class QDockWidget;
class QLabel;
class QComboBox;

/**
 * @class MainWindow
 * @brief Main application window
 *
 * Orchestrates all UI components and connects them to the
 * backend services (SerialManager, ProtocolHandler, DataBuffer).
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~MainWindow() override;

protected:
    /**
     * @brief Handle close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;

private slots:
    /**
     * @brief Handle connect request from settings widget
     * @param settings Serial settings
     */
    void onConnectRequested(const SerialSettings &settings);
    
    /**
     * @brief Handle disconnect request
     */
    void onDisconnectRequested();
    
    /**
     * @brief Handle connection state change
     * @param connected True if connected
     * @param message Status message
     */
    void onConnectionStateChanged(bool connected, const QString &message);
    
    /**
     * @brief Handle serial error
     * @param error Error message
     */
    void onSerialError(const QString &error);
    
    /**
     * @brief Handle raw bytes from serial
     * @param data Raw bytes
     */
    void onRawBytesReceived(const QByteArray &data);
    
    /**
     * @brief Handle parsed data packet
     * @param packet Parsed packet
     */
    void onDataParsed(const GenericDataPacket &packet);
    
    /**
     * @brief Handle send data from terminal
     * @param data Data to send
     */
    void onSendData(const QByteArray &data);
    
    /**
     * @brief Handle protocol selection change
     * @param index Combo box index
     */
    void onProtocolChanged(int index);
    
    /**
     * @brief Show about dialog
     */
    void showAbout();

private:
    /**
     * @brief Set up the UI
     */
    void setupUi();
    
    /**
     * @brief Set up menus
     */
    void setupMenus();
    
    /**
     * @brief Set up status bar
     */
    void setupStatusBar();
    
    /**
     * @brief Initialize the protocol handler
     */
    void initProtocolHandler();
    
    /**
     * @brief Connect all signals and slots
     */
    void connectSignals();
    
    /**
     * @brief Save window state to settings
     */
    void saveSettings();
    
    /**
     * @brief Load window state from settings
     */
    void loadSettings();

    // UI Components
    QTabWidget *m_tabWidget = nullptr;
    QDockWidget *m_settingsDock = nullptr;
    
    SerialSettingsWidget *m_serialSettings = nullptr;
    TerminalWidget *m_terminal = nullptr;
    PlotterWidget *m_plotter = nullptr;
    
    // Status bar widgets
    QLabel *m_statusLabel = nullptr;
    QLabel *m_packetCountLabel = nullptr;
    QComboBox *m_protocolCombo = nullptr;
    
    // Backend components
    std::unique_ptr<ProtocolHandler> m_protocolHandler;
    std::unique_ptr<DataBuffer> m_dataBuffer;
    
    // State
    quint64 m_packetCount = 0;
};

#endif // MAINWINDOW_H
