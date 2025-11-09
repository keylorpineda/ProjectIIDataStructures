#include "ProjectIIDataStructures.h"
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHeaderView>
#include <QMessageBox>
#include <QPointF>
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QSizeF>
#include <QStringList>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <cmath>

ProjectIIDataStructures::ProjectIIDataStructures(QWidget *parent)
    : QMainWindow(parent), stationIdValidator(new QIntValidator(1, 999999, this)), graphScene(new QGraphicsScene(this))
{
    ui.setupUi(this);
    ui.graphView->setScene(graphScene);
    ui.graphView->setAutoFitEnabled(true);
    ui.graphView->setFocusPolicy(Qt::StrongFocus);
    ui.stationIdEdit->setValidator(stationIdValidator);
    manager.initialize();
    setupUiBehavior();
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
    connect(ui.addRouteButton, &QPushButton::clicked, this, [this]() {
        auto selection = selectedRoute();
        if (selection.first <= 0 || selection.second <= 0)
        {
            displayError("Seleccione estaciones válidas para la ruta.");
            return;
        }
        double time = ui.routeTimeSpin->value();
        if (manager.addRoute(selection.first, selection.second, time))
        {
            displayMessage("Ruta registrada exitosamente.");
            refreshRoutes();
            refreshClosures();
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
    graphScene->clear();

    const auto stations = manager.getStations();
    if (stations.empty())
    {
        graphScene->addText("No hay estaciones registradas.");
        QRectF rect = graphScene->itemsBoundingRect().adjusted(-40.0, -40.0, 40.0, 40.0);
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

    std::unordered_map<int, QPointF> positions;
    const double pi = std::acos(-1.0);
    const double twoPi = 2.0 * pi;
    const double baseRadius = 140.0;
    const double growthFactor = 34.0;
    double radius = baseRadius + growthFactor * std::log1p(static_cast<double>(stations.size()));
    double sceneWidth = radius * 2.0 + 320.0;
    double sceneHeight = radius * 2.0 + 260.0;
    const double centerX = sceneWidth / 2.0;
    const double centerY = sceneHeight / 2.0;

    for (size_t i = 0; i < stations.size(); ++i)
    {
        double ratio = stations.size() == 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(stations.size());
        double angle = ratio * twoPi - pi / 2.0;
        double radialMod = 0.85 + 0.18 * std::sin(angle * 3.0);
        double x = centerX + radius * radialMod * std::cos(angle);
        double y = centerY + radius * (0.9 + 0.12 * std::cos(angle * 2.0)) * std::sin(angle);
        positions[stations[i].getId()] = QPointF(x, y);
    }

    const double nodeRadius = 24.0;
    const QColor nodeFill(52, 152, 219);
    const QColor nodeBorder(19, 33, 60);
    const QColor edgeColor(41, 128, 185);
    const QColor edgeLabelBg(255, 255, 255, 180);
    const QColor closedColor(231, 76, 60);
    const QColor closureLabelBg(231, 76, 60, 110);
    const QFont edgeFont("Sans Serif", 9, QFont::DemiBold);
    const QFont nodeFont("Sans Serif", 9, QFont::Bold);

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
        QGraphicsLineItem *line = graphScene->addLine(QLineF(fromPoint, toPoint), pen);
        line->setZValue(0);

        QPointF mid((fromPoint.x() + toPoint.x()) / 2.0, (fromPoint.y() + toPoint.y()) / 2.0);
        double dx = toPoint.x() - fromPoint.x();
        double dy = toPoint.y() - fromPoint.y();
        double length = std::hypot(dx, dy);
        QPointF offset(0.0, 0.0);
        if (length > 0.0)
        {
            double offsetDistance = 18.0;
            offset = QPointF(-dy / length * offsetDistance, dx / length * offsetDistance);
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
        auto *labelBackground = graphScene->addRoundedRect(bgRect, 6.0, 6.0, QPen(Qt::NoPen), QBrush(backgroundColor));
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

        QRectF haloRect = ellipseRect.adjusted(-10.0, -10.0, 10.0, 10.0);
        auto *halo = graphScene->addEllipse(haloRect, Qt::NoPen, QBrush(QColor(255, 255, 255, 28)));
        halo->setZValue(1.5);

        QRadialGradient gradient(point, nodeRadius * 1.25, point);
        gradient.setColorAt(0.0, nodeFill.lighter(125));
        gradient.setColorAt(0.7, nodeFill);
        gradient.setColorAt(1.0, nodeBorder.darker(180));

        auto *node = graphScene->addEllipse(ellipseRect, QPen(nodeBorder, 2.4), QBrush(gradient));
        node->setZValue(2.0);

        QString labelText = QString::number(station.getId()) + "\n" + station.getName();
        auto *textItem = graphScene->addText(labelText, nodeFont);
        QRectF textRect = textItem->boundingRect();
        textItem->setDefaultTextColor(Qt::white);
        textItem->setPos(point.x() - textRect.width() / 2.0, point.y() - textRect.height() / 2.0);
        textItem->setZValue(2.5);
    }

    QRectF bounding = graphScene->itemsBoundingRect().adjusted(-80.0, -80.0, 80.0, 80.0);
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
