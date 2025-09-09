#include "seismic_canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QDebug>
#include <algorithm>
#include <cmath>

SeismicCanvas::SeismicCanvas(QWidget *parent)
    : QWidget(parent)
    , m_sampleInterval(0.0)
    , m_vmin(0.0f)
    , m_vmax(1.0f)
    , m_pixmapValid(false)
    , m_backgroundColor(Qt::black)
    , m_selectionMode(POINT_BY_POINT)
    , m_dragging(false)
    , m_selectionPen(QPen(Qt::red, 2, Qt::SolidLine))
    , m_previewPen(QPen(Qt::red, 2, Qt::DashLine))
{
    setMinimumSize(400, 300);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void SeismicCanvas::setData(const QVector<QVector<float>>& data, double sample_interval)
{
    m_data = data;
    m_processedData = data;
    m_sampleInterval = sample_interval;
    
    clearSelection();

    if (!m_data.isEmpty() && !m_data[0].isEmpty()) {
        calculateDataRange();
        updatePixmap();
    } else {
        m_pixmapValid = false;
    }
    
    update();
}

void SeismicCanvas::updateProcessedData(const QVector<QVector<float>>& new_data)
{
    if (new_data.isEmpty() || m_data.isEmpty() || new_data.size() != m_data.size() || 
        new_data[0].size() != m_data[0].size()) {
        qWarning() << "updateProcessedData: Invalid data size provided.";
        return;
    }
    
    m_processedData = new_data;
    updatePixmap();
    update();
}

void SeismicCanvas::setSelectionMode(SelectionMode mode)
{
    if (m_selectionMode != mode) {
        m_selectionMode = mode;
        clearSelection();
    }
}

void SeismicCanvas::clearSelection()
{
    m_points.clear();
    m_rectStart = QPointF();
    m_dragging = false;
    update();
}


void SeismicCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.fillRect(rect(), m_backgroundColor);
    
    if (m_pixmapValid) {
        painter.drawPixmap(0, 0, m_pixmap);
    }
    
    drawSelection(painter);
}

void SeismicCanvas::mousePressEvent(QMouseEvent *event)
{
    if (m_data.isEmpty()) {
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (m_selectionMode == POINT_BY_POINT) {
            // If this is the first click for a new polygon, the old one is cleared implicitly
            // because finalizeSelection doesn't reset m_points
            m_points.append(pixelToDataCoords(event->pos()));
            update();
        } else if (m_selectionMode == RECTANGLE) {
            clearSelection();
            m_rectStart = pixelToDataCoords(event->pos());
            m_dragging = true;
        }
    } else if (event->button() == Qt::RightButton) {
        if (m_selectionMode == POINT_BY_POINT) {
            if (m_points.size() >= 2) {
                finalizeSelection();
            } else {
                clearSelection();
            }
        } else if (m_selectionMode == RECTANGLE) {
            if (m_points.size() == 2) {
                finalizeSelection();
            } else {
                clearSelection();
            }
        }
    }
}

void SeismicCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging && m_selectionMode == RECTANGLE) {
        m_dragging = false;
        QPointF endPoint = pixelToDataCoords(event->pos());
        m_points.append(m_rectStart);
        m_points.append(endPoint);
        // Don't call finalizeSelection() here - just update display
        update();
    }
}

void SeismicCanvas::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_dragging && m_selectionMode == RECTANGLE) {
        update();
    }
}

void SeismicCanvas::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_selectionMode == POINT_BY_POINT && m_points.size() >= 2) {
            finalizeSelection();
            event->accept();
        }
    } else if (event->key() == Qt::Key_Escape) {
        clearSelection();
        event->accept();
    }
    else {
        QWidget::keyPressEvent(event);
    }
}

void SeismicCanvas::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updatePixmap();
    update();
}

void SeismicCanvas::updatePixmap()
{
    if (m_processedData.isEmpty() || m_processedData[0].isEmpty() || width() <= 0 || height() <= 0) {
        m_pixmapValid = false;
        return;
    }
    
    m_pixmap = QPixmap(size());
    m_pixmap.fill(m_backgroundColor);
    
    QPainter painter(&m_pixmap);
    drawData(painter);
    
    m_pixmapValid = true;
}

void SeismicCanvas::drawData(QPainter& painter)
{
    int n_traces = m_processedData.size();
    int n_samples = m_processedData[0].size();
    
    QImage image(size(), QImage::Format_RGB32);
    image.fill(m_backgroundColor);

    float trace_step = static_cast<float>(width()) / n_traces;
    float sample_step = static_cast<float>(height()) / n_samples;

    for (int y = 0; y < height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        int sample_idx = static_cast<int>(y / sample_step);
        if (sample_idx >= n_samples) continue;

        for (int x = 0; x < width(); ++x) {
            int trace_idx = static_cast<int>(x / trace_step);
            if (trace_idx >= n_traces) continue;
            
            QColor color = amplitudeToColor(m_processedData[trace_idx][sample_idx]);
            line[x] = color.rgb();
        }
    }
    painter.drawImage(0, 0, image);
}

void SeismicCanvas::drawSelection(QPainter& painter)
{
    if (m_points.isEmpty() && !m_dragging) {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing);

    if (m_selectionMode == RECTANGLE) {
        if (m_dragging) {
            painter.setPen(m_previewPen);
            QPoint currentMousePos = this->mapFromGlobal(QCursor::pos());
            QRectF previewRect(dataCoordsToPixel(m_rectStart), currentMousePos);
            painter.drawRect(previewRect.normalized());
        } else if (m_points.size() == 2) {
            // Draw main selection area
            painter.setPen(m_selectionPen);
            QRectF finalRect(dataCoordsToPixel(m_points[0]), dataCoordsToPixel(m_points[1]));
            painter.drawRect(finalRect.normalized());
            
        }
    } else if (m_selectionMode == POINT_BY_POINT) {
        painter.setPen(m_selectionPen);
        if (m_points.size() < 1) return;

        QPolygonF polygon;
        for (const auto& point : m_points) {
            polygon << dataCoordsToPixel(point);
        }

        for(const auto& pixel_point : polygon) {
             painter.drawEllipse(pixel_point, 4, 4);
        }

        if (polygon.size() > 1) {
            painter.drawPolyline(polygon);
        }
        
        if (polygon.size() >= 3) {
            painter.setPen(m_previewPen);
            painter.drawLine(polygon.last(), polygon.first());
        }
        
    }
}

QPointF SeismicCanvas::dataCoordsToPixel(const QPointF& dataPoint) const
{
    if (m_data.isEmpty() || m_data[0].isEmpty()) return QPointF();

    const qreal n_traces = m_data.size();
    const qreal max_time = (m_data[0].size() - 1) * m_sampleInterval * 1000.0;
    
    if (n_traces <= 1 || max_time < 1e-9) return QPointF(0, dataPoint.y() / max_time * (height() - 1));

    qreal x = (dataPoint.x() / (n_traces - 1)) * (width() - 1);
    qreal y = (dataPoint.y() / max_time) * (height() - 1);

    return QPointF(x, y);
}

QPointF SeismicCanvas::pixelToDataCoords(const QPointF& pixelPoint) const
{
    if (m_data.isEmpty() || m_data[0].isEmpty()) return QPointF();

    const qreal n_traces = m_data.size();
    const qreal max_time = (m_data[0].size() - 1) * m_sampleInterval * 1000.0;

    if (width() <= 1 || height() <= 1) return QPointF();

    qreal trace = (pixelPoint.x() / (width() - 1)) * (n_traces - 1);
    qreal time_ms = (pixelPoint.y() / (height() - 1)) * max_time;
    
    trace = std::max(0.0, std::min(n_traces - 1, trace));
    time_ms = std::max(0.0, std::min(max_time, time_ms));

    return QPointF(trace, time_ms);
}

void SeismicCanvas::finalizeSelection()
{
    if (m_points.size() < 2) {
        clearSelection();
        return;
    }
    
    emit windowSelected(m_points);
    update();
}

void SeismicCanvas::calculateDataRange()
{
    if (m_data.isEmpty() || m_data[0].isEmpty()) return;

    QVector<float> flat_data;
    flat_data.reserve(m_data.size() * m_data[0].size());
    for(const auto& trace : m_data) {
        for(float sample : trace) {
            flat_data.append(sample);
        }
    }

    if(flat_data.isEmpty()) return;

    std::sort(flat_data.begin(), flat_data.end());

    int p1_index = std::min(static_cast<int>(flat_data.size() - 1), static_cast<int>(flat_data.size() * 0.01));
    int p99_index = std::min(static_cast<int>(flat_data.size() - 1), static_cast<int>(flat_data.size() * 0.99));

    m_vmin = flat_data[p1_index];
    m_vmax = flat_data[p99_index];

    qDebug() << "Data range (1-99 percentile):" << m_vmin << "to" << m_vmax;
}

QColor SeismicCanvas::amplitudeToColor(float amplitude) const
{
    float range = m_vmax - m_vmin;
    if (range < 1e-9) {
        return QColor(128, 128, 128);
    }
    float clamped_amp = std::max(m_vmin, std::min(m_vmax, amplitude));
    float normalized = (clamped_amp - m_vmin) / range;
    
    int gray = static_cast<int>(normalized * 255);
    return QColor(gray, gray, gray);
}
