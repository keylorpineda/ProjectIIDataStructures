#include "ProjectIIDataStructures.h"
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QGraphicsSceneMouseEvent>
#include <QPushButton>
#include <QPointF>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QSignalBlocker>
#include <QTransform>
#include <optional>
#include <QRadialGradient>
#include <QSizeF>
#include <QStringList>
#include <QtMath>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <cmath>
#include <vector>

namespace
{
constexpr int kStationItemRole = 1;
}

ProjectIIDataStructures::ProjectIIDataStructures(QWidget *parent)
    : QMainWindow(parent),
      stationIdValidator(new QIntValidator(1, 999999, this)),
      graphScene(new QGraphicsScene(this)),
      mapPixmapItem(nullptr)
{
    ui.setupUi(this);
    ui.graphView->setScene(graphScene);
    graphScene->installEventFilter(this);
    ui.graphView->setAutoFitEnabled(true);
    ui.graphView->setFocusPolicy(Qt::StrongFocus);
    ui.stationIdEdit->setValidator(stationIdValidator);
    initializeMapStorage();
    manager.initialize();
    setupUiBehavior();
    loadPersistedMap();
    refreshAll();
    displayMessage("Bienvenido al gestor de transporte La Mancha.");
}

ProjectIIDataStructures::~ProjectIIDataStructures()
{
}

void ProjectIIDataStructures::closeEvent(QCloseEvent *event)
{
    manager.saveData();
    QMainWindow::closeEvent(event);
}

void ProjectIIDataStructures::setupUiBehavior()
{
    ui.stationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.routeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.routeTimeSpin->setDecimals(2);
    ui.routeTimeSpin->setMinimum(0.10);
    ui.routeTimeSpin->setSingleStep(0.50);
    ui.routeTimeSpin->setToolTip(tr("Ingrese manualmente el tiempo estimado de la ruta en minutos."));
    connect(ui.zoomInButton, &QPushButton::clicked, ui.graphView, &InteractiveGraphicsView::zoomIn);
    connect(ui.zoomOutButton, &QPushButton::clicked, ui.graphView, &InteractiveGraphicsView::zoomOut);
    connect(ui.resetViewButton, &QPushButton::clicked, ui.graphView, &InteractiveGraphicsView::resetToFit);
    connect(ui.graphView, &InteractiveGraphicsView::scenePointActivated, this, &ProjectIIDataStructures::promptAddStationAt);
    connect(ui.addStationButton, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        int id = ui.stationIdEdit->text().toInt(&ok);
        QString name = ui.stationNameEdit->text();
        if (!ok)
        {
            displayError("Ingrese un código válido.");
            return;
        }
        if (manager.addStation(id, name))
        {
            displayMessage("Estación registrada con éxito.");
            ui.stationIdEdit->clear();
            ui.stationNameEdit->clear();
            refreshAll();
        }
        else
        {
            displayError("No se pudo registrar la estación.");
        }
    });
    connect(ui.removeStationButton, &QPushButton::clicked, this, [this]() {
        int id = selectedStationId();
        if (id <= 0)
        {
            displayError("Seleccione o ingrese una estación válida.");
            return;
        }
        if (manager.removeStation(id))
        {
            displayMessage("Estación eliminada correctamente.");
            refreshAll();
        }
        else
        {
            displayError("No se pudo eliminar la estación.");
        }
    });
    connect(ui.routeOriginCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int /*index*/) {
        updateRouteTimeSuggestion();
    });
    connect(ui.routeDestinationCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this](int /*index*/) {
        updateRouteTimeSuggestion();
    });
    connect(ui.addRouteButton, &QPushButton::clicked, this, [this]() {
        auto selection = selectedRoute();
        if (selection.first <= 0 || selection.second <= 0)
        {
            displayError("Seleccione estaciones válidas para la ruta.");
            return;
        }
        std::optional<double> computed = manager.calculateRouteWeightFromCoordinates(selection.first, selection.second);
        std::optional<double> time = computed;
        if (!time.has_value())
        {
            double manual = ui.routeTimeSpin->value();
            if (manual <= 0.0)
            {
                displayError("Ingrese un tiempo válido mayor que cero.");
                return;
            }
            time = manual;
        }
        if (manager.addRoute(selection.first, selection.second, time))
        {
            if (computed.has_value())
            {
                QSignalBlocker blocker(ui.routeTimeSpin);
                ui.routeTimeSpin->setValue(computed.value());
            }
            QString message;
            if (computed.has_value())
            {
                message = QString("Ruta registrada exitosamente. Tiempo calculado: %1 minutos.")
                              .arg(QString::number(computed.value(), 'f', 2));
            }
            else
            {
                message = "Ruta registrada exitosamente.";
            }
            displayMessage(message);
            refreshRoutes();
            refreshClosures();
            updateRouteTimeSuggestion();
        }
        else
        {
            displayError("No se pudo registrar la ruta.");
        }
    });
    connect(ui.removeRouteButton, &QPushButton::clicked, this, [this]() {
        auto selection = selectedRoute();
        if (selection.first <= 0 || selection.second <= 0)
        {
            if (ui.routeTable->currentRow() >= 0)
            {
                int row = ui.routeTable->currentRow();
                bool okFrom = false;
                bool okTo = false;
                int fromId = ui.routeTable->item(row, 0)->data(Qt::UserRole).toInt(&okFrom);
                int toId = ui.routeTable->item(row, 1)->data(Qt::UserRole).toInt(&okTo);
                if (okFrom && okTo)
                {
                    selection = {fromId, toId};
                }
            }
        }
        if (selection.first <= 0 || selection.second <= 0)
        {
            displayError("Seleccione una ruta para eliminar.");
            return;
        }
        if (manager.removeRoute(selection.first, selection.second))
        {
            displayMessage("Ruta eliminada correctamente.");
            refreshRoutes();
            refreshClosures();
        }
        else
        {
            displayError("No se pudo eliminar la ruta.");
        }
    });
    connect(ui.loadClosuresButton, &QPushButton::clicked, this, [this]() {
        manager.reloadClosures();
        refreshClosures();
        displayMessage("Cierres de vía actualizados.");
    });
    connect(ui.loadMapButton, &QPushButton::clicked, this, [this]() {
        QString filter = "Imágenes (*.png *.jpg *.jpeg *.bmp *.gif *.webp)";
        QString filePath = QFileDialog::getOpenFileName(this, tr("Seleccionar mapa"), QString(), filter);
        if (filePath.isEmpty())
        {
            return;
        }
        QPixmap pixmap(filePath);
        if (pixmap.isNull())
        {
            displayError("No se pudo cargar la imagen seleccionada.");
            return;
        }
        QSize originalSize = pixmap.size();
        if (!applyMapPixmap(pixmap, true))
        {
            displayError("No se pudo preparar el mapa seleccionado.");
            return;
        }
        if (!persistMapPixmap(loadedMapPixmap))
        {
            displayError("No se pudo guardar la imagen seleccionada como mapa.");
            return;
        }
        QSize scaledSize = loadedMapPixmap.size();
        QString detail = QString(" (%1×%2 px)")
                              .arg(QString::number(scaledSize.width()),
                                   QString::number(scaledSize.height()));
        if (scaledSize != originalSize)
        {
            detail = QString(" (%1×%2 px, ajustado desde %3×%4 px)")
                         .arg(QString::number(scaledSize.width()),
                              QString::number(scaledSize.height()),
                              QString::number(originalSize.width()),
                              QString::number(originalSize.height()));
        }
        displayMessage(QString("Mapa de fondo actualizado y almacenado%1.").arg(detail));
    });
    connect(ui.clearMapButton, &QPushButton::clicked, this, [this]() {
        if (!mapIsActive())
        {
            displayMessage("No hay un mapa guardado para eliminar.");
            return;
        }
        clearStoredMap();
        ui.graphView->clearBackgroundImage();
        refreshGraphVisualization();
        displayMessage("Se eliminó el mapa guardado y se restauró el fondo predeterminado.");
    });
    connect(ui.runTraversalButton, &QPushButton::clicked, this, [this]() {
        int startId = ui.traversalStartCombo->currentData().toInt();
        if (startId <= 0)
        {
            displayError("Seleccione una estación de inicio.");
            return;
        }
        std::vector<int> result;
        if (ui.traversalCombo->currentText() == "BFS")
        {
            result = manager.runBfs(startId);
        }
        else
        {
            result = manager.runDfs(startId);
        }
        if (result.empty())
        {
            ui.resultText->setText("No hay recorrido disponible.");
            displayError("No fue posible realizar el recorrido.");
        }
        else
        {
            QString text = QString("Recorrido %1:\n%2").arg(ui.traversalCombo->currentText(), joinStations(result));
            ui.resultText->setText(text);
            displayMessage("Recorrido generado con éxito.");
        }
    });
    connect(ui.runShortestButton, &QPushButton::clicked, this, [this]() {
        int startId = ui.shortestStartCombo->currentData().toInt();
        int endId = ui.shortestEndCombo->currentData().toInt();
        if (startId <= 0 || endId <= 0)
        {
            displayError("Seleccione estaciones válidas.");
            return;
        }
        PathDetail detail;
        if (ui.shortestCombo->currentText() == "Dijkstra")
        {
            detail = manager.runDijkstra(startId, endId);
        }
        else
        {
            detail = manager.runFloyd(startId, endId);
        }
        if (detail.stations.empty() || !std::isfinite(detail.total))
        {
            ui.resultText->setText("No se encontró un camino disponible.");
            displayError("No existe una ruta válida entre las estaciones seleccionadas.");
        }
        else
        {
            QString text = QString("Ruta óptima (%1 minutos):\n%2").arg(QString::number(detail.total, 'f', 2), joinStations(detail.stations));
            ui.resultText->setText(text);
            displayMessage("Ruta calculada correctamente.");
        }
    });
    connect(ui.runPrimButton, &QPushButton::clicked, this, [this]() {
        TreeDetail detail = manager.runPrim();
        if (detail.edges.empty())
        {
            ui.resultText->setText("No se pudo construir un árbol de expansión.");
            displayError("No hay conexiones suficientes para generar el árbol.");
            return;
        }
        QStringList lines;
        lines << QString("Árbol mínimo Prim (%1 minutos):").arg(QString::number(detail.total, 'f', 2));
        for (const auto &edge : detail.edges)
        {
            lines << QString("%1 ⇄ %2 : %3").arg(QString::number(edge.from), QString::number(edge.to), QString::number(edge.weight, 'f', 2));
        }
        ui.resultText->setText(lines.join('\n'));
        displayMessage("Árbol mínimo Prim generado.");
    });
    connect(ui.runKruskalButton, &QPushButton::clicked, this, [this]() {
        TreeDetail detail = manager.runKruskal();
        if (detail.edges.empty())
        {
            ui.resultText->setText("No se pudo construir un árbol de expansión.");
            displayError("No hay conexiones suficientes para generar el árbol.");
            return;
        }
        QStringList lines;
        lines << QString("Árbol mínimo Kruskal (%1 minutos):").arg(QString::number(detail.total, 'f', 2));
        for (const auto &edge : detail.edges)
        {
            lines << QString("%1 ⇄ %2 : %3").arg(QString::number(edge.from), QString::number(edge.to), QString::number(edge.weight, 'f', 2));
        }
        ui.resultText->setText(lines.join('\n'));
        displayMessage("Árbol mínimo Kruskal generado.");
    });
    connect(ui.showStationsButton, &QPushButton::clicked, this, [this]() {
        QString report = manager.buildStationsReport();
        ui.reportsText->setText(report);
        displayMessage("Estaciones ordenadas mostradas.");
    });
    connect(ui.showRoutesButton, &QPushButton::clicked, this, [this]() {
        QString report = manager.buildRoutesReport();
        ui.reportsText->setText(report);
        displayMessage("Rutas y cierres mostrados.");
    });
    connect(ui.exportTraversalsButton, &QPushButton::clicked, this, [this]() {
        QString report = manager.exportTraversals();
        ui.reportsText->setText(report);
        displayMessage("Recorridos exportados a archivo.");
    });
    connect(ui.saveReportButton, &QPushButton::clicked, this, [this]() {
        QString content = ui.reportsText->toPlainText();
        if (content.trimmed().isEmpty())
        {
            displayError("No hay información para guardar.");
            return;
        }
        manager.saveReportContent(content);
        displayMessage("Reporte almacenado exitosamente.");
    });
}

void ProjectIIDataStructures::refreshStations()
{
    auto stations = manager.getStations();
    ui.stationTable->setRowCount(static_cast<int>(stations.size()));
    int row = 0;
    for (const auto &station : stations)
    {
        auto idItem = new QTableWidgetItem(QString::number(station.getId()));
        idItem->setData(Qt::UserRole, station.getId());
        auto nameItem = new QTableWidgetItem(station.getName());
        ui.stationTable->setItem(row, 0, idItem);
        ui.stationTable->setItem(row, 1, nameItem);
        row++;
    }
    refreshGraphVisualization();
}

void ProjectIIDataStructures::refreshRoutes()
{
    auto routes = manager.getRoutes();
    ui.routeTable->setRowCount(static_cast<int>(routes.size()));
    int row = 0;
    for (const auto &route : routes)
    {
        auto originItem = new QTableWidgetItem(QString::number(route.from));
        originItem->setData(Qt::UserRole, route.from);
        originItem->setText(QString::number(route.from) + " - " + manager.getStationName(route.from));
        auto destinationItem = new QTableWidgetItem(QString::number(route.to));
        destinationItem->setData(Qt::UserRole, route.to);
        destinationItem->setText(QString::number(route.to) + " - " + manager.getStationName(route.to));
        auto timeItem = new QTableWidgetItem(QString::number(route.weight, 'f', 2));
        ui.routeTable->setItem(row, 0, originItem);
        ui.routeTable->setItem(row, 1, destinationItem);
        ui.routeTable->setItem(row, 2, timeItem);
        row++;
    }
    refreshGraphVisualization();
}

void ProjectIIDataStructures::refreshClosures()
{
    auto closures = manager.getClosures();
    ui.closuresList->clear();
    if (closures.empty())
    {
        ui.closuresInfoLabel->setText("No hay cierres cargados");
        return;
    }
    ui.closuresInfoLabel->setText(QString("Cierres activos: %1").arg(closures.size()));
    for (const auto &closure : closures)
    {
        QString text = QString("%1 ⇄ %2").arg(QString::number(closure.first), QString::number(closure.second));
        ui.closuresList->addItem(text);
    }
    refreshGraphVisualization();
}

void ProjectIIDataStructures::refreshCombos()
{
    auto stations = manager.getStations();
    auto fillCombo = [](QComboBox *combo, const std::vector<Station> &stationList) {
        combo->clear();
        combo->addItem("Seleccione", 0);
        for (const auto &station : stationList)
        {
            combo->addItem(QString::number(station.getId()) + " - " + station.getName(), station.getId());
        }
    };
    fillCombo(ui.routeOriginCombo, stations);
    fillCombo(ui.routeDestinationCombo, stations);
    fillCombo(ui.traversalStartCombo, stations);
    fillCombo(ui.shortestStartCombo, stations);
    fillCombo(ui.shortestEndCombo, stations);
    updateRouteTimeSuggestion();
}

void ProjectIIDataStructures::refreshAll()
{
    refreshStations();
    refreshRoutes();
    refreshClosures();
    refreshCombos();
}

void ProjectIIDataStructures::refreshGraphVisualization()
{
    if (!graphScene || !ui.graphView)
    {
        return;
    }
    bool hasMap = mapIsActive();
    graphScene->clear();
    mapPixmapItem = nullptr;
    if (hasMap)
    {
        addMapItemToScene();
    }

    const auto stations = manager.getStations();
    if (stations.empty())
    {
        if (!hasMap)
        {
            graphScene->addText("No hay estaciones registradas.");
        }
        QRectF rect;
        if (hasMap && !mapSceneRect.isNull())
        {
            rect = mapSceneRect;
        }
        else
        {
            rect = graphScene->itemsBoundingRect().adjusted(-40.0, -40.0, 40.0, 40.0);
            if (rect.isNull())
            {
                rect = QRectF(-200.0, -150.0, 400.0, 300.0);
            }
        }
        graphScene->setSceneRect(rect);
        ui.graphView->setContentRect(rect, true);
        return;
    }

    const auto routes = manager.getRoutes();
    const auto closures = manager.getClosures();

    std::set<std::pair<int, int>> closureSet;
    for (const auto &closure : closures)
    {
        int a = std::min(closure.first, closure.second);
        int b = std::max(closure.first, closure.second);
        closureSet.insert({a, b});
    }

    std::unordered_map<int, QPointF> fallbackPositions;
    std::unordered_map<int, QPointF> positions;
    const double pi = std::acos(-1.0);
    const double twoPi = 2.0 * pi;
    const double baseRadius = 140.0;
    const double growthFactor = 34.0;
    double radius = baseRadius + growthFactor * std::log1p(static_cast<double>(stations.size()));
    if (hasMap && !mapSceneRect.isNull())
    {
        double maxRadius = 0.45 * std::min(mapSceneRect.width(), mapSceneRect.height());
        radius = std::min(radius, maxRadius);
    }
    double sceneWidth = hasMap && !mapSceneRect.isNull() ? mapSceneRect.width() : radius * 2.0 + 320.0;
    double sceneHeight = hasMap && !mapSceneRect.isNull() ? mapSceneRect.height() : radius * 2.0 + 260.0;
    QPointF layoutCenter = hasMap && !mapSceneRect.isNull() ? mapSceneRect.center() : QPointF(sceneWidth / 2.0, sceneHeight / 2.0);

    for (size_t i = 0; i < stations.size(); ++i)
    {
        double ratio = stations.size() == 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(stations.size());
        double angle = ratio * twoPi - pi / 2.0;
        double radialMod = 0.85 + 0.18 * std::sin(angle * 3.0);
        double x = layoutCenter.x() + radius * radialMod * std::cos(angle);
        double y = layoutCenter.y() + radius * (0.9 + 0.12 * std::cos(angle * 2.0)) * std::sin(angle);
        fallbackPositions[stations[i].getId()] = QPointF(x, y);
    }

    for (const auto &station : stations)
    {
        if (hasMap && station.hasCoordinates())
        {
            positions[station.getId()] = station.getPosition();
        }
        else
        {
            positions[station.getId()] = fallbackPositions[station.getId()];
        }
    }

    const double nodeRadius = 14.0;
    const QColor nodeFill(52, 152, 219);
    const QColor nodeBorder(19, 33, 60);
    const QColor edgeColor(41, 128, 185);
    const QColor edgeLabelBg(255, 255, 255, 180);
    const QColor closedColor(231, 76, 60);
    const QColor closureLabelBg(231, 76, 60, 110);
    const QFont edgeFont("Sans Serif", 8, QFont::DemiBold);
    const QFont nodeFont("Sans Serif", 8, QFont::DemiBold);

    // Draw edges first so that nodes remain on top.
    for (const auto &edge : routes)
    {
        auto fromIt = positions.find(edge.from);
        auto toIt = positions.find(edge.to);
        if (fromIt == positions.end() || toIt == positions.end())
        {
            continue;
        }
        QPointF fromPoint = fromIt->second;
        QPointF toPoint = toIt->second;
        std::pair<int, int> normalizedPair = {std::min(edge.from, edge.to), std::max(edge.from, edge.to)};
        bool isClosed = closureSet.find(normalizedPair) != closureSet.end();

        QPen pen(isClosed ? closedColor : edgeColor, isClosed ? 3.8 : 2.6);
        if (isClosed)
        {
            pen.setStyle(Qt::DashLine);
        }
        std::vector<QPointF> pathPoints;
        pathPoints.push_back(fromPoint);

        if (hasMap && mapSceneRect.contains(fromPoint) && mapSceneRect.contains(toPoint))
        {
            QPointF corner1(fromPoint.x(), toPoint.y());
            QPointF corner2(toPoint.x(), fromPoint.y());
            bool corner1Valid = mapSceneRect.contains(corner1);
            bool corner2Valid = mapSceneRect.contains(corner2);
            if (corner1Valid || corner2Valid)
            {
                QPointF chosenCorner;
                if (corner1Valid && corner2Valid)
                {
                    double length1 = QLineF(fromPoint, corner1).length() + QLineF(corner1, toPoint).length();
                    double length2 = QLineF(fromPoint, corner2).length() + QLineF(corner2, toPoint).length();
                    chosenCorner = length1 <= length2 ? corner1 : corner2;
                }
                else
                {
                    chosenCorner = corner1Valid ? corner1 : corner2;
                }
                pathPoints.push_back(chosenCorner);
            }
        }

        pathPoints.push_back(toPoint);

        QPainterPath path(pathPoints.front());
        for (size_t i = 1; i < pathPoints.size(); ++i)
        {
            path.lineTo(pathPoints[i]);
        }

        auto *pathItem = graphScene->addPath(path, pen);
        pathItem->setZValue(0);

        std::vector<double> segmentLengths;
        segmentLengths.reserve(pathPoints.size() - 1);
        double totalLength = 0.0;
        for (size_t i = 1; i < pathPoints.size(); ++i)
        {
            double len = QLineF(pathPoints[i - 1], pathPoints[i]).length();
            segmentLengths.push_back(len);
            totalLength += len;
        }

        QPointF mid = fromPoint;
        QPointF direction(1.0, 0.0);
        if (totalLength > 0.0)
        {
            double half = totalLength / 2.0;
            double accumulated = 0.0;
            for (size_t i = 0; i < segmentLengths.size(); ++i)
            {
                double len = segmentLengths[i];
                if (accumulated + len >= half)
                {
                    double ratio = (half - accumulated) / len;
                    QPointF startPoint = pathPoints[i];
                    QPointF endPoint = pathPoints[i + 1];
                    mid = startPoint + (endPoint - startPoint) * ratio;
                    direction = endPoint - startPoint;
                    break;
                }
                accumulated += len;
            }
        }
        if (qFuzzyIsNull(direction.x()) && qFuzzyIsNull(direction.y()))
        {
            direction = toPoint - fromPoint;
        }
        double length = std::hypot(direction.x(), direction.y());
        QPointF offset(0.0, 0.0);
        if (length > 0.0)
        {
            QPointF normal(-direction.y() / length, direction.x() / length);
            double offsetDistance = 18.0;
            offset = normal * offsetDistance;
        }
        QString weightText = QString::number(edge.weight, 'f', 1);
        if (isClosed)
        {
            weightText.append(" (cerrado)");
        }
        auto *weightItem = graphScene->addText(weightText, edgeFont);
        QRectF textRect = weightItem->boundingRect();
        QPointF labelPos = mid + offset - QPointF(textRect.width() / 2.0, textRect.height() / 2.0);
        QRectF bgRect(labelPos - QPointF(6.0, 4.0), textRect.size() + QSizeF(12.0, 8.0));
        QColor backgroundColor = isClosed ? closureLabelBg : edgeLabelBg;

        // Use a simple rectangle background (rounded path caused compilation issues on some setups)
        auto *labelBackground = graphScene->addRect(bgRect, QPen(Qt::NoPen), QBrush(backgroundColor));
        labelBackground->setZValue(0.8);
        weightItem->setDefaultTextColor(isClosed ? QColor(255, 255, 255) : QColor(45, 52, 54));
        weightItem->setPos(labelPos);
        weightItem->setZValue(1.2);
    }

    for (const auto &station : stations)
    {
        auto positionIt = positions.find(station.getId());
        if (positionIt == positions.end())
        {
            continue;
        }
        QPointF point = positionIt->second;
        QRectF ellipseRect(point.x() - nodeRadius, point.y() - nodeRadius, nodeRadius * 2.0, nodeRadius * 2.0);

        QRectF haloRect = ellipseRect.adjusted(-6.0, -6.0, 6.0, 6.0);
        auto *halo = graphScene->addEllipse(haloRect, Qt::NoPen, QBrush(QColor(255, 255, 255, 28)));
        halo->setZValue(1.5);
        halo->setData(kStationItemRole, station.getId());
        halo->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);

        QRadialGradient gradient(point, nodeRadius * 1.35, point);
        gradient.setColorAt(0.0, nodeFill.lighter(125));
        gradient.setColorAt(0.7, nodeFill);
        gradient.setColorAt(1.0, nodeBorder.darker(180));

        auto *node = graphScene->addEllipse(ellipseRect, QPen(nodeBorder, 1.6), QBrush(gradient));
        node->setZValue(2.0);
        node->setData(kStationItemRole, station.getId());
        node->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
        node->setCursor(Qt::PointingHandCursor);

        QString labelText = QString::number(station.getId()) + "\n" + station.getName();
        auto *textItem = graphScene->addText(labelText, nodeFont);
        QRectF textRect = textItem->boundingRect();
        textItem->setDefaultTextColor(Qt::white);
        textItem->setPos(point.x() - textRect.width() / 2.0, point.y() - textRect.height() / 2.0);
        textItem->setZValue(2.5);
        textItem->setData(kStationItemRole, station.getId());
        textItem->setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    }

    QRectF bounding = graphScene->itemsBoundingRect();
    if (hasMap)
    {
        bounding = bounding.united(mapSceneRect);
        bounding = bounding.adjusted(-10.0, -10.0, 10.0, 10.0);
    }
    else
    {
        bounding = bounding.adjusted(-80.0, -80.0, 80.0, 80.0);
    }
    graphScene->setSceneRect(bounding);
    ui.graphView->setContentRect(bounding, !ui.graphView->hasUserAdjusted());
}

void ProjectIIDataStructures::displayMessage(const QString &text)
{
    statusBar()->showMessage(text, 4000);
}

void ProjectIIDataStructures::displayError(const QString &text)
{
    statusBar()->showMessage(text, 6000);
    QMessageBox::warning(this, "Aviso", text);
}

int ProjectIIDataStructures::selectedStationId() const
{
    if (ui.stationTable->currentRow() >= 0)
    {
        auto item = ui.stationTable->item(ui.stationTable->currentRow(), 0);
        if (item)
        {
            return item->data(Qt::UserRole).toInt();
        }
    }
    bool ok = false;
    int id = ui.stationIdEdit->text().toInt(&ok);
    return ok ? id : -1;
}

std::pair<int, int> ProjectIIDataStructures::selectedRoute() const
{
    int origin = ui.routeOriginCombo->currentData().toInt();
    int destination = ui.routeDestinationCombo->currentData().toInt();
    return {origin, destination};
}

QString ProjectIIDataStructures::joinStations(const std::vector<int> &ids) const
{
    if (ids.empty())
    {
        return "No hay estaciones en la ruta.";
    }
    QStringList parts;
    for (int id : ids)
    {
        QString name = manager.getStationName(id);
        if (name.isEmpty())
        {
            parts << QString::number(id);
        }
        else
        {
            parts << QString::number(id) + " - " + name;
        }
    }
    return parts.join(" → ");
}

void ProjectIIDataStructures::initializeMapStorage()
{
    QString baseDir = manager.dataDirectory();
    if (baseDir.isEmpty())
    {
        baseDir = QCoreApplication::applicationDirPath();
    }
    QDir base(baseDir);
    if (!base.exists())
    {
        base.mkpath(".");
    }
    mapStorageDirectory = base.filePath("mapas");
    QDir storage(mapStorageDirectory);
    if (!storage.exists())
    {
        storage.mkpath(".");
    }
    mapStoredFile = storage.filePath("mapa_fondo.png");
}

void ProjectIIDataStructures::loadPersistedMap()
{
    if (mapStoredFile.isEmpty())
    {
        return;
    }
    if (!QFile::exists(mapStoredFile))
    {
        return;
    }
    QPixmap pixmap(mapStoredFile);
    if (pixmap.isNull())
    {
        return;
    }
    QSize originalSize = pixmap.size();
    if (!applyMapPixmap(pixmap, false))
    {
        return;
    }
    if (loadedMapPixmap.size() != originalSize)
    {
        persistMapPixmap(loadedMapPixmap);
    }
}

bool ProjectIIDataStructures::persistMapPixmap(const QPixmap &pixmap)
{
    if (mapStoredFile.isEmpty())
    {
        return false;
    }
    QDir storage(mapStorageDirectory);
    if (!storage.exists())
    {
        if (!storage.mkpath("."))
        {
            return false;
        }
    }
    if (QFile::exists(mapStoredFile) && !QFile::remove(mapStoredFile))
    {
        return false;
    }
    return pixmap.save(mapStoredFile, "PNG");
}

bool ProjectIIDataStructures::applyMapPixmap(const QPixmap &pixmap, bool forceFit)
{
    if (pixmap.isNull())
    {
        return false;
    }
    QSize originalSize = pixmap.size();
    QPixmap processed = prepareMapPixmap(pixmap);
    QSize processedSize = processed.size();
    if (processedSize.isEmpty())
    {
        return false;
    }
    if (originalSize.width() > 0 && originalSize.height() > 0 && processedSize != originalSize)
    {
        double scaleX = static_cast<double>(processedSize.width()) / static_cast<double>(originalSize.width());
        double scaleY = static_cast<double>(processedSize.height()) / static_cast<double>(originalSize.height());
        manager.scaleStationPositions(scaleX, scaleY);
    }
    loadedMapPixmap = processed;
    mapSceneRect = QRectF(QPointF(0.0, 0.0), QSizeF(processedSize));
    ui.graphView->clearBackgroundImage();
    refreshGraphVisualization();
    ui.graphView->setContentRect(mapSceneRect, forceFit);
    updateRouteTimeSuggestion();
    return true;
}

QPixmap ProjectIIDataStructures::prepareMapPixmap(const QPixmap &pixmap) const
{
    if (!ui.graphView || pixmap.isNull())
    {
        return pixmap;
    }
    // Preserve the original pixel dimensions so the map keeps its native
    // resolution and appears crisp inside the scene.
    return pixmap;
}

void ProjectIIDataStructures::clearStoredMap()
{
    if (!mapStoredFile.isEmpty() && QFile::exists(mapStoredFile))
    {
        QFile::remove(mapStoredFile);
    }
    loadedMapPixmap = QPixmap();
    mapSceneRect = QRectF();
    if (mapPixmapItem)
    {
        if (graphScene)
        {
            graphScene->removeItem(mapPixmapItem);
        }
        delete mapPixmapItem;
        mapPixmapItem = nullptr;
    }
    updateRouteTimeSuggestion();
}

void ProjectIIDataStructures::addMapItemToScene()
{
    if (!graphScene || loadedMapPixmap.isNull())
    {
        return;
    }
    mapPixmapItem = graphScene->addPixmap(loadedMapPixmap);
    mapPixmapItem->setZValue(-1000.0);
    mapPixmapItem->setPos(0.0, 0.0);
    mapPixmapItem->setTransformationMode(Qt::SmoothTransformation);
    mapPixmapItem->setCacheMode(QGraphicsItem::ItemCoordinateCache);
}

bool ProjectIIDataStructures::mapIsActive() const
{
    return !loadedMapPixmap.isNull();
}

bool ProjectIIDataStructures::pointWithinMap(const QPointF &point) const
{
    return mapIsActive() && !mapSceneRect.isNull() && mapSceneRect.contains(point);
}

void ProjectIIDataStructures::updateRouteTimeSuggestion()
{
    if (!ui.routeTimeSpin)
    {
        return;
    }
    int originId = ui.routeOriginCombo->currentData().toInt();
    int destinationId = ui.routeDestinationCombo->currentData().toInt();
    auto computed = manager.calculateRouteWeightFromCoordinates(originId, destinationId);
    const QString autoTip = tr("Tiempo calculado automáticamente a partir de las coordenadas del mapa.");
    if (computed.has_value() && computed.value() > 0.0)
    {
        QSignalBlocker blocker(ui.routeTimeSpin);
        ui.routeTimeSpin->setValue(computed.value());
        ui.routeTimeSpin->setEnabled(false);
        ui.routeTimeSpin->setToolTip(autoTip);
    }
    else
    {
        ui.routeTimeSpin->setEnabled(true);
        ui.routeTimeSpin->setToolTip(tr("Ingrese manualmente el tiempo estimado de la ruta en minutos."));
    }
}

void ProjectIIDataStructures::promptAddStationAt(const QPointF &scenePos)
{
    if (!mapIsActive())
    {
        return;
    }
    if (!pointWithinMap(scenePos))
    {
        displayError("Seleccione un punto dentro del mapa para registrar la parada.");
        return;
    }

    bool ok = false;
    int id = QInputDialog::getInt(this, tr("Nueva estación"), tr("Código de estación:"), 1, 1, 999999, 1, &ok);
    if (!ok)
    {
        return;
    }
    QString name = QInputDialog::getText(this, tr("Nueva estación"), tr("Nombre de la estación:"), QLineEdit::Normal, QString(), &ok).trimmed();
    if (!ok)
    {
        return;
    }
    if (name.isEmpty())
    {
        displayError("El nombre de la estación no puede estar vacío.");
        return;
    }
    if (manager.addStation(id, name, std::optional<QPointF>(scenePos)))
    {
        displayMessage("Estación agregada correctamente en el mapa.");
        refreshAll();
    }
    else
    {
        displayError("No se pudo registrar la estación. Verifique que el código no exista.");
    }
}

bool ProjectIIDataStructures::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == graphScene)
    {
        if (event->type() == QEvent::GraphicsSceneMousePress)
        {
            auto *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);
            if (mouseEvent->button() == Qt::RightButton)
            {
                QGraphicsItem *item = graphScene->itemAt(mouseEvent->scenePos(), QTransform());
                if (item)
                {
                    QVariant data = item->data(kStationItemRole);
                    if (data.isValid())
                    {
                        handleStationRemovalRequest(data.toInt());
                        return true;
                    }
                }
            }
        }
        else if (event->type() == QEvent::GraphicsSceneMouseDoubleClick)
        {
            auto *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                QGraphicsItem *item = graphScene->itemAt(mouseEvent->scenePos(), QTransform());
                if (item)
                {
                    QVariant data = item->data(kStationItemRole);
                    if (data.isValid())
                    {
                        handleStationRemovalRequest(data.toInt());
                        return true;
                    }
                }
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void ProjectIIDataStructures::handleStationRemovalRequest(int stationId)
{
    if (stationId <= 0)
    {
        return;
    }
    QString stationName = manager.getStationName(stationId);
    QString prompt = stationName.isEmpty()
                          ? QString("¿Desea eliminar la estación %1?").arg(stationId)
                          : QString("¿Desea eliminar la estación %1 - %2?").arg(stationId).arg(stationName);
    auto response = QMessageBox::question(this, tr("Eliminar estación"), prompt, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (response != QMessageBox::Yes)
    {
        return;
    }
    if (manager.removeStation(stationId))
    {
        displayMessage(QString("Estación %1 eliminada correctamente.").arg(stationId));
        refreshAll();
    }
    else
    {
        displayError("No se pudo eliminar la estación seleccionada.");
    }
}
