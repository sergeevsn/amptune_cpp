#ifndef SEISMIC_CANVAS_H
#define SEISMIC_CANVAS_H

#include <QWidget>
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QPen>
#include <QKeyEvent>

class SeismicCanvas : public QWidget
{
    Q_OBJECT

public:
    enum SelectionMode {
        POINT_BY_POINT,
        RECTANGLE
    };

    explicit SeismicCanvas(QWidget *parent = nullptr);

    void setData(const QVector<QVector<float>>& data, double sample_interval);
    void updateProcessedData(const QVector<QVector<float>>& new_data);

    void setSelectionMode(SelectionMode mode);
    void clearSelection();
    

signals:
    void windowSelected(const QVector<QPointF>& points);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updatePixmap();
    void drawData(QPainter& painter);
    void drawSelection(QPainter& painter);

    QPointF dataCoordsToPixel(const QPointF& dataPoint) const;
    QPointF pixelToDataCoords(const QPointF& pixelPoint) const;

    void finalizeSelection();
    void calculateDataRange();
    QColor amplitudeToColor(float amplitude) const;

    // Data
    QVector<QVector<float>> m_data;
    QVector<QVector<float>> m_processedData;
    double m_sampleInterval; // in seconds
    float m_vmin;
    float m_vmax;

    // Rendering
    QPixmap m_pixmap;
    bool m_pixmapValid;
    QColor m_backgroundColor;

    // Selection
    SelectionMode m_selectionMode;
    QVector<QPointF> m_points; // Stores points in coordinates (trace, time_ms)
    QPointF m_rectStart;
    bool m_dragging;
    QPen m_selectionPen;
    QPen m_previewPen;
    
};

#endif // SEISMIC_CANVAS_H