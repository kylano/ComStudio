/**
 * @file main.cpp
 * @brief Application entry point for ComStudio Serial Terminal
 *
 * ComStudio is a Qt-based serial terminal application that supports
 * multiple protocols, real-time plotting, and data analysis.
 */

#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include "ui/MainWindow.h"

/**
 * @brief Load and apply QSS stylesheet from resources
 * @param app The QApplication instance
 * @return true if stylesheet was loaded successfully
 */
bool loadStylesheet(QApplication &app)
{
    QFile styleFile(":/styles/app.qss");
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&styleFile);
        QString stylesheet = stream.readAll();
        app.setStyleSheet(stylesheet);
        styleFile.close();
        return true;
    }
    qWarning() << "Failed to load stylesheet from resources";
    return false;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Application metadata
    QCoreApplication::setApplicationName("ComStudio");
    QCoreApplication::setApplicationVersion("0.1");
    QCoreApplication::setOrganizationName("ComStudio");
    
    // Load dark theme stylesheet
    loadStylesheet(app);
    
    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    return app.exec();
}
