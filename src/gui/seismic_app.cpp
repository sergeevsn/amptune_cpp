#include "seismic_app.h"
#include "ioutils/segy_reader.h"
#include "ioutils/segy_writer.h"
#include "amplify/amplify.h"
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpacerItem>
#include <QDebug>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Using declarations for cleaner code
using ioutils::SegyReader;
using ioutils::SegyWriter;

SeismicApp::SeismicApp(QWidget *parent)
    : QMainWindow(parent)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_leftPanel(nullptr)
    , m_rightPanel(nullptr)
    , m_loadBtn(nullptr)
    , m_saveBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_clearSelectionBtn(nullptr)
    , m_undoBtn(nullptr)
    , m_redoBtn(nullptr)
    , m_selectionModeCombo(nullptr)
    , m_scaleFactorLabel(nullptr)
    , m_scaleFactorSpin(nullptr)
    , m_transitionTracesSpin(nullptr)
    , m_transitionTimeSpin(nullptr)
    , m_transitionModeCombo(nullptr)
    , m_dataInfoLabel(nullptr)
    , m_historyInfoLabel(nullptr)
    , m_canvas(nullptr)
    , m_sampleInterval(0.0)
    , m_historyIndex(-1)
    , m_segyReader(nullptr)
    , m_segyWriter(nullptr)
{
    setWindowTitle("Seismic Data Amplification Tuning Tool");
    setGeometry(100, 100, 1400, 800);
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    initUI();
}

SeismicApp::~SeismicApp()
{
    delete m_segyReader;
    // m_segyWriter is created on stack in saveFile, so no need to delete it here
}

void SeismicApp::initUI()
{
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);
    
    m_mainLayout = new QHBoxLayout(m_centralWidget);
    m_leftPanel = new QVBoxLayout();
    m_rightPanel = new QVBoxLayout();
    
    QHBoxLayout* controlLayout = new QHBoxLayout();
    
    m_loadBtn = new QPushButton("Load SEG-Y File");
    m_saveBtn = new QPushButton("Save Processed Data");
    m_resetBtn = new QPushButton("Reset");
    m_clearSelectionBtn = new QPushButton("Clear Selection");
    m_undoBtn = new QPushButton("Undo");
    m_redoBtn = new QPushButton("Redo");
    
    connect(m_loadBtn, &QPushButton::clicked, this, &SeismicApp::loadFile);
    connect(m_saveBtn, &QPushButton::clicked, this, &SeismicApp::saveFile);
    connect(m_resetBtn, &QPushButton::clicked, this, &SeismicApp::resetData);
    connect(m_clearSelectionBtn, &QPushButton::clicked, this, &SeismicApp::clearCurrentSelection);
    connect(m_undoBtn, &QPushButton::clicked, this, &SeismicApp::undoAction);
    connect(m_redoBtn, &QPushButton::clicked, this, &SeismicApp::redoAction);
    
    m_saveBtn->setEnabled(false);
    m_resetBtn->setEnabled(false);
    m_clearSelectionBtn->setEnabled(false);
    m_undoBtn->setEnabled(false);
    m_redoBtn->setEnabled(false);
    
    controlLayout->addWidget(m_loadBtn);
    controlLayout->addWidget(m_saveBtn);
    controlLayout->addWidget(m_resetBtn);
    controlLayout->addWidget(m_clearSelectionBtn);
    controlLayout->addWidget(m_undoBtn);
    controlLayout->addWidget(m_redoBtn);
    controlLayout->addStretch();
    
    m_leftPanel->addLayout(controlLayout);
    
    m_canvas = new SeismicCanvas();
    connect(m_canvas, &SeismicCanvas::windowSelected, this, &SeismicApp::onWindowSelected);
    m_leftPanel->addWidget(m_canvas);
    
    QWidget* controlPanel = createControlPanel();
    m_rightPanel->addWidget(controlPanel);
    
    m_mainLayout->addLayout(m_leftPanel, 3);
    m_mainLayout->addLayout(m_rightPanel, 1);
    
}

QWidget* SeismicApp::createControlPanel()
{
    QWidget* panel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(panel);
    
    QGroupBox* selectionGroup = new QGroupBox("Selection Mode");
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);
    
    m_selectionModeCombo = new QComboBox();
    m_selectionModeCombo->addItems({"Point by Point", "Rectangle"});
    connect(m_selectionModeCombo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
            this, &SeismicApp::onSelectionModeChanged);
    
    selectionLayout->addWidget(new QLabel("Mode:"));
    selectionLayout->addWidget(m_selectionModeCombo);
    selectionGroup->setLayout(selectionLayout);
    layout->addWidget(selectionGroup);
    
    QGroupBox* paramsGroup = new QGroupBox("Amplification Parameters");
    QVBoxLayout* paramsLayout = new QVBoxLayout(paramsGroup);
    
    m_scaleFactorLabel = new QLabel("Scale Factor:");
    paramsLayout->addWidget(m_scaleFactorLabel);
    m_scaleFactorSpin = new QDoubleSpinBox();
    m_scaleFactorSpin->setRange(0.1, 20.0);
    m_scaleFactorSpin->setValue(2.0);
    m_scaleFactorSpin->setSingleStep(0.1);
    paramsLayout->addWidget(m_scaleFactorSpin);
    
    paramsLayout->addWidget(new QLabel("Transition Traces:"));
    m_transitionTracesSpin = new QSpinBox();
    m_transitionTracesSpin->setRange(0, 100);
    m_transitionTracesSpin->setValue(5);
    paramsLayout->addWidget(m_transitionTracesSpin);
    
    paramsLayout->addWidget(new QLabel("Transition Time (ms):"));
    m_transitionTimeSpin = new QDoubleSpinBox();
    m_transitionTimeSpin->setRange(0.0, 1000.0);
    m_transitionTimeSpin->setValue(20.0);
    m_transitionTimeSpin->setSingleStep(5.0);
    paramsLayout->addWidget(m_transitionTimeSpin);
    
    paramsLayout->addWidget(new QLabel("Transition Mode:"));
    m_transitionModeCombo = new QComboBox();
    m_transitionModeCombo->addItems({"inside", "outside"});
    paramsLayout->addWidget(m_transitionModeCombo);
    
    
    paramsGroup->setLayout(paramsLayout);
    layout->addWidget(paramsGroup);
    
    QGroupBox* infoGroup = new QGroupBox("Data Info");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    
    m_dataInfoLabel = new QLabel("No data loaded");
    m_dataInfoLabel->setWordWrap(true);
    infoLayout->addWidget(m_dataInfoLabel);
    infoGroup->setLayout(infoLayout);
    layout->addWidget(infoGroup);
    
    QGroupBox* historyGroup = new QGroupBox("History");
    QVBoxLayout* historyLayout = new QVBoxLayout(historyGroup);
    
    m_historyInfoLabel = new QLabel("No history");
    m_historyInfoLabel->setWordWrap(true);
    historyLayout->addWidget(m_historyInfoLabel);
    historyGroup->setLayout(historyLayout);
    layout->addWidget(historyGroup);
    
    layout->addStretch();
    
    return panel;
}

void SeismicApp::loadFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Load SEG-Y File", "", 
                                                   "SEG-Y Files (*.sgy *.segy)");
    if (filePath.isEmpty()) return;
    
    try {
        m_lastSelectedPoints.clear();
        
        delete m_segyReader;
        m_segyReader = new SegyReader(filePath.toStdString());
        
        const auto& traces = m_segyReader->getAllTraces();
        m_sampleInterval = m_segyReader->getDt();
        
        m_originalData = convertSegyDataToQt(traces);
        m_currentData = m_originalData;
        m_originalFilePath = filePath;
        
        m_history.clear();
        m_historyIndex = -1;
        saveToHistory(m_originalData, "Original data loaded");
        
        m_canvas->setData(m_originalData, m_sampleInterval);
        updateDataInfo();
        
        m_saveBtn->setEnabled(true);
        m_resetBtn->setEnabled(true);
        m_clearSelectionBtn->setEnabled(true);
        updateUndoRedoButtons();
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", 
                             QString("Failed to load SEGY file: %1").arg(e.what()));
    }
}

void SeismicApp::saveFile()
{
    if (m_currentData.isEmpty() || m_originalFilePath.isEmpty()) return;

    QString filePath = QFileDialog::getSaveFileName(this, "Save Processed SEG-Y File", 
                                                    m_originalFilePath,
                                                    "SEG-Y Files (*.sgy *.segy)");
    if (filePath.isEmpty()) return;
    
    try {
        auto segyData = convertQtDataToSegy(m_currentData);
        SegyWriter writer(filePath.toStdString(), m_originalFilePath.toStdString());
        writer.writeFile(segyData, m_sampleInterval);
        QMessageBox::information(this, "Success", QString("File saved successfully to:\n%1").arg(filePath));
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Save Error", QString("Failed to save file:\n%1").arg(e.what()));
    }
}

void SeismicApp::resetData()
{
    if (m_originalData.isEmpty()) return;
    
    m_lastSelectedPoints.clear();
    m_canvas->clearSelection();
    
    m_history.clear();
    m_historyIndex = -1;
    
    m_currentData = m_originalData;
    saveToHistory(m_currentData, "Data reset to original");
    
    m_canvas->setData(m_originalData, m_sampleInterval);
}

void SeismicApp::clearCurrentSelection()
{
    m_canvas->clearSelection();
    m_lastSelectedPoints.clear();
}

void SeismicApp::undoAction()
{
    if (m_historyIndex > 0) {
        m_lastSelectedPoints.clear();
        m_canvas->clearSelection();
        
        m_historyIndex--;
        const auto& state = m_history[m_historyIndex];
        m_currentData = state.data;
        m_canvas->updateProcessedData(m_currentData);
        updateUndoRedoButtons();
    }
}

void SeismicApp::redoAction()
{
    if (m_historyIndex < m_history.size() - 1) {
        m_historyIndex++;
        const auto& state = m_history[m_historyIndex];
        m_currentData = state.data;
        m_canvas->updateProcessedData(m_currentData);
        
        updateUndoRedoButtons();
        m_lastSelectedPoints.clear();
        m_canvas->clearSelection();
    }
}

void SeismicApp::onWindowSelected(const QVector<QPointF>& points)
{
    if (points.isEmpty() || m_history.isEmpty()) return;
    
    m_lastSelectedPoints = points;

    qDebug() << "=== NEW WINDOW SELECTION ===";
    qDebug() << "History index:" << m_historyIndex;
    qDebug() << "History size:" << m_history.size();
    
    // Use current data as base for new processing (not original)
    const auto& baseData = m_history[m_historyIndex].data;
    qDebug() << "Using current processed data as base for new window";
    processWindow(points, true, &baseData);
}

void SeismicApp::onSelectionModeChanged(const QString& modeText)
{
    auto mode = (modeText == "Point by Point") ? 
                SeismicCanvas::POINT_BY_POINT : 
                SeismicCanvas::RECTANGLE;
    m_canvas->setSelectionMode(mode);
}



void SeismicApp::processWindow(const QVector<QPointF>& points, bool addToHistory, 
                              const QVector<QVector<float>>* baseData)
{
    if (baseData == nullptr || baseData->isEmpty()) {
        qWarning() << "processWindow called with no base data.";
        return;
    }
    
    QApplication::setOverrideCursor(Qt::WaitCursor);
    
    try {
        // Calculate RMS amplitude BEFORE processing
        double rmsBefore = calculateRMSInWindow(points, *baseData);
        qDebug() << "=== DEBUG: Processing Window ===";
        qDebug() << "Points count:" << points.size();
        qDebug() << "RMS amplitude BEFORE processing:" << rmsBefore;
        qDebug() << "Base data info - traces:" << baseData->size() << "samples:" << (baseData->isEmpty() ? 0 : baseData->at(0).size());
        
        // Output points for debugging
        qDebug() << "Window points:";
        for (int i = 0; i < points.size(); ++i) {
            qDebug() << "  Point" << i << ":" << points[i].x() << "traces," << points[i].y() << "ms";
        }
        
        std::vector<amplify::Point> amplifyPoints;
        amplifyPoints.reserve(points.size());
        for (const auto& point : points) {
            amplifyPoints.emplace_back(static_cast<int>(point.x()), point.y());
        }
        
        qDebug() << "Amplify points:";
        for (size_t i = 0; i < amplifyPoints.size(); ++i) {
            qDebug() << "  AmplifyPoint" << i << ":" << amplifyPoints[i].trace << "traces," << amplifyPoints[i].time_ms << "ms";
        }
        
        auto segyData = convertQtDataToSegy(*baseData);
        
        float dt_ms = m_sampleInterval * 1000.0f;
        auto mode = amplify::ProcessingMode::SCALE;
        auto transitionMode = (m_transitionModeCombo->currentText() == "inside") ? 
                              amplify::TransitionMode::INSIDE : amplify::TransitionMode::OUTSIDE;
        
        qDebug() << "Processing parameters:";
        qDebug() << "  Mode: scale";
        qDebug() << "  Scale factor:" << m_scaleFactorSpin->value();
        qDebug() << "  Transition traces:" << m_transitionTracesSpin->value();
        qDebug() << "  Transition time:" << m_transitionTimeSpin->value() << "ms";
        qDebug() << "  Transition mode:" << m_transitionModeCombo->currentText();
        qDebug() << "  dt_ms:" << dt_ms;
        
        amplify::AmplifyResult result = amplify::amplifySeismicWindow(
            segyData, dt_ms, amplifyPoints, mode,
            m_scaleFactorSpin->value(), m_transitionTracesSpin->value(),
            m_transitionTimeSpin->value(), transitionMode,
            0, 0.0  // align parameters not used in scale mode
        );
        
        m_currentData = convertSegyDataToQt(result.output_data);
        
        // Calculate RMS amplitude AFTER processing
        double rmsAfter = calculateRMSInWindow(points, m_currentData);
        qDebug() << "RMS amplitude AFTER processing:" << rmsAfter;
        qDebug() << "RMS change ratio:" << (rmsAfter / rmsBefore);
        
        // Debug: check how many points are in the window mask
        int windowPointsCount = 0;
        for (size_t i = 0; i < result.window_indices.size(); ++i) {
            for (size_t j = 0; j < result.window_indices[i].size(); ++j) {
                if (result.window_indices[i][j]) {
                    windowPointsCount++;
                }
            }
        }
        qDebug() << "Window mask points count:" << windowPointsCount;
        qDebug() << "=== END DEBUG ===";
        
        m_canvas->updateProcessedData(m_currentData);
        
        // Clear selection after processing
        m_canvas->clearSelection();
        m_lastSelectedPoints.clear();
        
        QString description = "Amplify: scale";
        
        if (addToHistory) {
            saveToHistory(m_currentData, description);
        } else if (m_historyIndex >= 0) {
            m_history[m_historyIndex].data = m_currentData;
            m_history[m_historyIndex].description = description;
            updateHistoryInfo();
        }
        
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Processing Error", QString("An error occurred during processing:\n%1").arg(e.what()));
    }
    
    QApplication::restoreOverrideCursor();
}

void SeismicApp::saveToHistory(const QVector<QVector<float>>& data, const QString& description)
{
    if (m_historyIndex < m_history.size() - 1) {
        m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
    }
    
    m_history.append({data, description});
    
    if (m_history.size() > MAX_HISTORY_SIZE) {
        m_history.removeFirst();
    }
    
    m_historyIndex = m_history.size() - 1;
    updateUndoRedoButtons();
}

void SeismicApp::updateUndoRedoButtons()
{
    m_undoBtn->setEnabled(m_historyIndex > 0);
    m_redoBtn->setEnabled(m_historyIndex < m_history.size() - 1);
    updateHistoryInfo();
}

void SeismicApp::updateHistoryInfo()
{
    if (!m_history.isEmpty() && m_historyIndex < m_history.size()) {
        const auto& currentDesc = m_history[m_historyIndex].description;
        QString historyText = QString("Current: %1\nHistory: %2/%3")
                             .arg(currentDesc).arg(m_historyIndex + 1).arg(m_history.size());
        m_historyInfoLabel->setText(historyText);
    } else {
        m_historyInfoLabel->setText("No history");
    }
}

void SeismicApp::updateDataInfo()
{
    if (m_originalData.isEmpty()) {
        m_dataInfoLabel->setText("No data loaded");
        return;
    }
    
    QFileInfo fileInfo(m_originalFilePath);
    QString infoText = QString("File: %1\nTraces: %2\nSamples: %3\nInterval: %4 ms")
                      .arg(fileInfo.fileName())
                      .arg(m_originalData.size())
                      .arg(m_originalData[0].size())
                      .arg(m_sampleInterval * 1000.0, 0, 'f', 2);
    m_dataInfoLabel->setText(infoText);
}

QVector<QVector<float>> SeismicApp::convertSegyDataToQt(const std::vector<std::vector<float>>& data) const
{
    QVector<QVector<float>> qtData;
    qtData.reserve(data.size());
    for (const auto& trace : data) {
        qtData.append(QVector<float>(trace.begin(), trace.end()));
    }
    return qtData;
}

std::vector<std::vector<float>> SeismicApp::convertQtDataToSegy(const QVector<QVector<float>>& data) const
{
    std::vector<std::vector<float>> segyData;
    segyData.reserve(data.size());
    for (const auto& trace : data) {
        segyData.emplace_back(trace.begin(), trace.end());
    }
    return segyData;
}

double SeismicApp::calculateRMSInWindow(const QVector<QPointF>& points, const QVector<QVector<float>>& data) const
{
    if (points.isEmpty() || data.isEmpty() || data[0].isEmpty()) {
        return 0.0;
    }
    
        // Determine window boundaries
    double minTrace = points[0].x();
    double maxTrace = points[0].x();
    double minTime = points[0].y();
    double maxTime = points[0].y();
    
    for (const auto& point : points) {
        minTrace = std::min(minTrace, point.x());
        maxTrace = std::max(maxTrace, point.x());
        minTime = std::min(minTime, point.y());
        maxTime = std::max(maxTime, point.y());
    }
    
    // Convert time to sample indices
    int minTraceIdx = static_cast<int>(std::max(0.0, std::min(static_cast<double>(data.size() - 1), minTrace)));
    int maxTraceIdx = static_cast<int>(std::max(0.0, std::min(static_cast<double>(data.size() - 1), maxTrace)));
    
    int minSampleIdx = static_cast<int>(std::max(0.0, minTime / (m_sampleInterval * 1000.0)));
    int maxSampleIdx = static_cast<int>(std::min(static_cast<double>(data[0].size() - 1), maxTime / (m_sampleInterval * 1000.0)));
    
    // Calculate RMS for all points in the window
    double sumSquares = 0.0;
    int count = 0;
    
    for (int traceIdx = minTraceIdx; traceIdx <= maxTraceIdx; ++traceIdx) {
        for (int sampleIdx = minSampleIdx; sampleIdx <= maxSampleIdx; ++sampleIdx) {
            if (traceIdx < data.size() && sampleIdx < data[traceIdx].size()) {
                double value = data[traceIdx][sampleIdx];
                sumSquares += value * value;
                count++;
            }
        }
    }
    
    if (count == 0) {
        return 0.0;
    }
    
    return std::sqrt(sumSquares / count);
}