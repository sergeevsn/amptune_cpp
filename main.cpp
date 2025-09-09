#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>

#include "src/gui/seismic_app.h"

/**
 * @brief Main entry point for the Seismic Data Amplification Tuning Tool
 * 
 * This application provides a GUI for processing seismic data with amplification
 * and alignment capabilities. It allows users to:
 * - Load SEGY files
 * - Visualize seismic data
 * - Select regions for processing
 * - Apply amplification or alignment
 * - Save processed results
 * - Undo/redo operations
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Seismic Amplification Tuning Tool");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Amptune");
    app.setOrganizationDomain("amptune.com");
    
    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Set dark theme palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    
    try {
        // Create and show main window
        SeismicApp window;
        window.show();
        
        // Print startup information
        qDebug() << "Seismic Amplification Tuning Tool started successfully";
        qDebug() << "Application version:" << app.applicationVersion();
        qDebug() << "Qt version:" << QT_VERSION_STR;
        
        return app.exec();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Application Error", 
                             QString("Failed to start application:\n%1").arg(e.what()));
        return 1;
    } catch (...) {
        QMessageBox::critical(nullptr, "Application Error", 
                             "Failed to start application: Unknown error");
        return 1;
    }
}

