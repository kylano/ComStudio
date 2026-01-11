/**
 * @file MainWindow.cpp
 * @brief Implementation of MainWindow
 */

#include "ui/MainWindow.h"
#include "ui/SerialSettingsWidget.h"
#include "ui/TerminalWidget.h"
#include "ui/PlotterWidget.h"
#include "core/SerialManager.h"
#include "core/ProtocolHandler.h"
#include "core/LineParser.h"
#include "models/DataBuffer.h"

#include <QTabWidget>
#include <QDockWidget>
#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>
#include <QLabel>
#include <QComboBox>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(tr("ComStudio - Serial Terminal"));
    setMinimumSize(1024, 768);
    
    // Initialize backend components
    m_protocolHandler = std::make_unique<ProtocolHandler>();
    m_dataBuffer = std::make_unique<DataBuffer>(10000);
    
    setupUi();
    setupMenus();
    setupStatusBar();
    initProtocolHandler();
    connectSignals();
    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUi()
{
    // Central widget with tabs
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setDocumentMode(true);
    setCentralWidget(m_tabWidget);
    
    // Terminal tab
    m_terminal = new TerminalWidget();
    m_tabWidget->addTab(m_terminal, tr("Terminal"));
    
    // Plotter tab
    m_plotter = new PlotterWidget();
    m_tabWidget->addTab(m_plotter, tr("Plotter"));
    
    // Serial settings dock
    m_settingsDock = new QDockWidget(tr("Serial Port"), this);
    m_settingsDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_settingsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    m_serialSettings = new SerialSettingsWidget();
    m_settingsDock->setWidget(m_serialSettings);
    
    addDockWidget(Qt::LeftDockWidgetArea, m_settingsDock);
}

void MainWindow::setupMenus()
{
    // File menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    
    QAction *refreshAction = fileMenu->addAction(tr("&Refresh Ports"));
    connect(refreshAction, &QAction::triggered, m_serialSettings, &SerialSettingsWidget::refreshPorts);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);
    
    // View menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    
    QAction *settingsAction = m_settingsDock->toggleViewAction();
    settingsAction->setText(tr("Serial Settings Panel"));
    viewMenu->addAction(settingsAction);
    
    viewMenu->addSeparator();
    
    QAction *clearTerminalAction = viewMenu->addAction(tr("Clear Terminal"));
    clearTerminalAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
    connect(clearTerminalAction, &QAction::triggered, m_terminal, &TerminalWidget::clear);
    
    QAction *clearPlotAction = viewMenu->addAction(tr("Clear Plot"));
    connect(clearPlotAction, &QAction::triggered, m_plotter, &PlotterWidget::clear);
    
    // Help menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    QAction *aboutAction = helpMenu->addAction(tr("&About ComStudio"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    
    QAction *aboutQtAction = helpMenu->addAction(tr("About &Qt"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::setupStatusBar()
{
    m_statusLabel = new QLabel(tr("Disconnected"));
    statusBar()->addWidget(m_statusLabel, 1);
    
    // Protocol selector in status bar
    QWidget *protocolWidget = new QWidget();
    QHBoxLayout *protocolLayout = new QHBoxLayout(protocolWidget);
    protocolLayout->setContentsMargins(0, 0, 0, 0);
    protocolLayout->setSpacing(4);
    
    protocolLayout->addWidget(new QLabel(tr("Protocol:")));
    m_protocolCombo = new QComboBox();
    m_protocolCombo->setMinimumWidth(120);
    connect(m_protocolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onProtocolChanged);
    protocolLayout->addWidget(m_protocolCombo);
    
    statusBar()->addPermanentWidget(protocolWidget);
    
    m_packetCountLabel = new QLabel(tr("Packets: 0"));
    statusBar()->addPermanentWidget(m_packetCountLabel);
}

void MainWindow::initProtocolHandler()
{
    // Register default line parser protocol
    auto lineParser = std::make_shared<LineParser>();
    m_protocolHandler->registerProtocol("line", lineParser);
    
    // Populate protocol combo
    m_protocolCombo->clear();
    for (const QString &id : m_protocolHandler->registeredProtocols()) {
        if (auto *proto = m_protocolHandler->protocol(id)) {
            m_protocolCombo->addItem(proto->name(), id);
        }
    }
}

void MainWindow::connectSignals()
{
    // Serial settings connections
    connect(m_serialSettings, &SerialSettingsWidget::connectRequested,
            this, &MainWindow::onConnectRequested);
    connect(m_serialSettings, &SerialSettingsWidget::disconnectRequested,
            this, &MainWindow::onDisconnectRequested);
    
    // SerialManager connections
    auto &serial = SerialManager::instance();
    connect(&serial, &SerialManager::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);
    connect(&serial, &SerialManager::errorOccurred,
            this, &MainWindow::onSerialError);
    connect(&serial, &SerialManager::rawBytesReady,
            this, &MainWindow::onRawBytesReceived);
    
    // Protocol handler connections
    connect(m_protocolHandler.get(), &ProtocolHandler::dataParsed,
            this, &MainWindow::onDataParsed);
    
    // Terminal send connection
    connect(m_terminal, &TerminalWidget::sendData,
            this, &MainWindow::onSendData);
    
    // Data buffer to UI connections
    connect(m_dataBuffer.get(), &DataBuffer::dataUpdated,
            m_plotter, &PlotterWidget::addData);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Disconnect before closing
    SerialManager::instance().disconnectPort();
    saveSettings();
    event->accept();
}

void MainWindow::onConnectRequested(const SerialSettings &settings)
{
    SerialManager::instance().connectPort(settings);
}

void MainWindow::onDisconnectRequested()
{
    SerialManager::instance().disconnectPort();
}

void MainWindow::onConnectionStateChanged(bool connected, const QString &message)
{
    m_serialSettings->onConnectionStateChanged(connected, message);
    m_statusLabel->setText(message);
    
    if (connected) {
        m_packetCount = 0;
        m_packetCountLabel->setText(tr("Packets: 0"));
        m_protocolHandler->resetParser();
    }
}

void MainWindow::onSerialError(const QString &error)
{
    statusBar()->showMessage(tr("Error: %1").arg(error), 5000);
}

void MainWindow::onRawBytesReceived(const QByteArray &data)
{
    // Send to protocol handler for parsing
    m_protocolHandler->processRawData(data);
    
    // Also send raw data to terminal in raw mode
    m_terminal->appendRawData(data);
}

void MainWindow::onDataParsed(const GenericDataPacket &packet)
{
    // Add to data buffer (will notify plotter)
    m_dataBuffer->addPacket(packet);
    
    // Update terminal in parsed mode
    m_terminal->appendPacket(packet);
    
    // Update packet counter
    m_packetCount++;
    m_packetCountLabel->setText(tr("Packets: %1").arg(m_packetCount));
}

void MainWindow::onSendData(const QByteArray &data)
{
    SerialManager::instance().sendData(data);
}

void MainWindow::onProtocolChanged(int index)
{
    QString protocolId = m_protocolCombo->itemData(index).toString();
    m_protocolHandler->setActiveProtocol(protocolId);
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About ComStudio"),
        tr("<h3>ComStudio v0.1</h3>"
           "<p>A professional serial terminal and data plotter.</p>"
           "<p>Built with Qt 6 and QCustomPlot.</p>"
           "<p>Features:</p>"
           "<ul>"
           "<li>Multi-protocol support</li>"
           "<li>Real-time data plotting</li>"
           "<li>High-performance line parser</li>"
           "<li>Modular architecture</li>"
           "</ul>"));
}

void MainWindow::saveSettings()
{
    QSettings settings("ComStudio", "ComStudio");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::loadSettings()
{
    QSettings settings("ComStudio", "ComStudio");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
}
