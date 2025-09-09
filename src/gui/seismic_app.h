#ifndef SEISMIC_APP_H
#define SEISMIC_APP_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QApplication>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QTimer>
#include <memory>

#include "seismic_canvas.h"
#include "../ioutils/segy_reader.h"
#include "../ioutils/segy_writer.h"

namespace amplify {
    struct AmplifyResult;
    enum class ProcessingMode;
    enum class TransitionMode;
}

/**
 * @brief Main application window for seismic data amplification tuning
 * 
 * This class provides the main GUI for the seismic data processing application.
 * It includes data loading, visualization, selection, and processing capabilities.
 */
class SeismicApp : public QMainWindow
{
    Q_OBJECT

public:
    explicit SeismicApp(QWidget *parent = nullptr);
    ~SeismicApp();

private slots:
    void loadFile();
    void saveFile();
    void resetData();
    void clearCurrentSelection();
    void undoAction();
    void redoAction();
    void onWindowSelected(const QVector<QPointF>& points);
    void onSelectionModeChanged(const QString& modeText);

private:
    // UI Components
    void initUI();
    QWidget* createControlPanel();
    void updateUndoRedoButtons();
    void updateDataInfo();
    void updateHistoryInfo();
    
    // Data Management
    void saveToHistory(const QVector<QVector<float>>& data, const QString& description);
    void processWindow(const QVector<QPointF>& points, bool addToHistory = true, 
                      const QVector<QVector<float>>* baseData = nullptr);
    
    // Data Conversion
    QVector<QPointF> convertPointsToAmplifyFormat(const QVector<QPointF>& points) const;
    QVector<QVector<float>> convertSegyDataToQt(const std::vector<std::vector<float>>& data) const;
    std::vector<std::vector<float>> convertQtDataToSegy(const QVector<QVector<float>>& data) const;
    
    // Debug functions
    double calculateRMSInWindow(const QVector<QPointF>& points, const QVector<QVector<float>>& data) const;
    
    // UI Elements
    QWidget* m_centralWidget;
    QHBoxLayout* m_mainLayout;
    QVBoxLayout* m_leftPanel;
    QVBoxLayout* m_rightPanel;
    
    // Control buttons
    QPushButton* m_loadBtn;
    QPushButton* m_saveBtn;
    QPushButton* m_resetBtn;
    QPushButton* m_clearSelectionBtn;
    QPushButton* m_undoBtn;
    QPushButton* m_redoBtn;
    
    // Selection controls
    QComboBox* m_selectionModeCombo;
    
    // Processing controls
    QLabel* m_scaleFactorLabel;
    QDoubleSpinBox* m_scaleFactorSpin;
    QSpinBox* m_transitionTracesSpin;
    QDoubleSpinBox* m_transitionTimeSpin;
    QComboBox* m_transitionModeCombo;
    
    // Info displays
    QLabel* m_dataInfoLabel;
    QLabel* m_historyInfoLabel;
    
    // Canvas
    SeismicCanvas* m_canvas;
    
    // Data
    QVector<QVector<float>> m_originalData;
    QVector<QVector<float>> m_currentData;
    double m_sampleInterval;
    QString m_originalFilePath;
    
    // History management
    struct HistoryEntry {
        QVector<QVector<float>> data;
        QString description;
    };
    QVector<HistoryEntry> m_history;
    int m_historyIndex;
    static const int MAX_HISTORY_SIZE = 20;
    
    // Selection
    QVector<QPointF> m_lastSelectedPoints;
    
    // Modules
    ioutils::SegyReader* m_segyReader;
    ioutils::SegyWriter* m_segyWriter;
};

#endif // SEISMIC_APP_H
