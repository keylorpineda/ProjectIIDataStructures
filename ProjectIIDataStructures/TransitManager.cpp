#include "TransitManager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QStringList>
#include <algorithm>
#include <cmath>
#include <limits>

TransitManager::TransitManager()
{
    dataManager.setBasePath(QCoreApplication::applicationDirPath());
}

void TransitManager::initialize()
{
    dataManager.load(tree, graph);
}

void TransitManager::saveData()
{
    dataManager.save(tree, graph);
}

QString TransitManager::dataDirectory() const
{
    return dataManager.getBasePath();
}

bool TransitManager::addStation(int id, const QString &name, const std::optional<QPointF> &position)
{
    QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty())
    {
        return false;
    }
    Station station(id, trimmedName);
    if (position.has_value())
    {
        station.setPosition(position.value());
    }
    if (!tree.insert(station))
    {
        return false;
    }
    if (!graph.addStation(station))
    {
        tree.remove(id);
        return false;
    }
    dataManager.appendReportLine(QString("%1 Estación agregada: %2 - %3").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"), QString::number(id), trimmedName));
    saveData();
    
    // Si la estación tiene coordenadas, generar rutas automáticas a estaciones cercanas
    if (position.has_value())
    {
        generateAutomaticRoutesForStation(id, 3); // Conectar con hasta 3 estaciones cercanas
    }
    
    return true;
}

bool TransitManager::removeStation(int id)
{
    Station station;
    if (!tree.find(id, station))
    {
        return false;
    }
    if (!tree.remove(id))
    {
        return false;
    }
    graph.removeStation(id);
    dataManager.appendReportLine(QString("%1 Estación eliminada: %2 - %3").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"), QString::number(id), station.getName()));
    saveData();
    return true;
}

bool TransitManager::addRoute(int fromId, int toId, const std::optional<double> &time)
{
    if (fromId == toId)
    {
        return false;
    }
    std::optional<double> weight = time;
    if (!weight.has_value() || weight.value() <= 0.0)
    {
        weight = calculateRouteWeightFromCoordinates(fromId, toId);
    }
    if (!weight.has_value() || weight.value() <= 0.0)
    {
        return false;
    }
    double finalWeight = weight.value();
    if (!graph.addConnection(fromId, toId, finalWeight))
    {
        return false;
    }
    dataManager.appendReportLine(QString("%1 Ruta agregada: %2 ⇄ %3 (%4 minutos)")
                                     .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"),
                                          QString::number(fromId),
                                          QString::number(toId),
                                          QString::number(finalWeight, 'f', 2)));
    saveData();
    return true;
}

bool TransitManager::removeRoute(int fromId, int toId)
{
    if (!graph.removeConnection(fromId, toId))
    {
        return false;
    }
    dataManager.appendReportLine(QString("%1 Ruta eliminada: %2 ⇄ %3").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"), QString::number(fromId), QString::number(toId)));
    saveData();
    return true;
}

std::optional<double> TransitManager::calculateRouteWeightFromCoordinates(int fromId, int toId) const
{
    if (fromId == toId)
    {
        return std::nullopt;
    }
    Station fromStation;
    Station toStation;
    if (!tree.find(fromId, fromStation) || !tree.find(toId, toStation))
    {
        return std::nullopt;
    }
    if (!fromStation.hasCoordinates() || !toStation.hasCoordinates())
    {
        return std::nullopt;
    }
    QPointF fromPos = fromStation.getPosition();
    QPointF toPos = toStation.getPosition();
    
    // Usar distancia Manhattan para simular rutas por calles
    double dx = std::abs(toPos.x() - fromPos.x());
    double dy = std::abs(toPos.y() - fromPos.y());
    double distance = dx + dy;
    
    if (!std::isfinite(distance) || distance <= 0.0)
    {
        return std::nullopt;
    }
    return distance;
}

std::vector<Station> TransitManager::getStations() const
{
    return tree.inOrder();
}

std::vector<GraphEdge> TransitManager::getRoutes() const
{
    return graph.getConnections();
}

std::vector<std::pair<int, int>> TransitManager::getClosures() const
{
    return graph.getClosures();
}

void TransitManager::reloadClosures()
{
    auto closures = dataManager.loadClosures();
    graph.applyClosures(closures);
}

std::vector<int> TransitManager::runBfs(int startId)
{
    return graph.bfs(startId);
}

std::vector<int> TransitManager::runDfs(int startId)
{
    return graph.dfs(startId);
}

PathDetail TransitManager::runDijkstra(int startId, int endId)
{
    return graph.dijkstra(startId, endId);
}

PathDetail TransitManager::runFloyd(int startId, int endId)
{
    return graph.floydWarshall(startId, endId);
}

TreeDetail TransitManager::runPrim()
{
    return graph.prim();
}

TreeDetail TransitManager::runKruskal()
{
    return graph.kruskal();
}

QString TransitManager::buildTraversalText(const QString &title, const std::vector<Station> &stations) const
{
    QStringList lines;
    lines << title;
    for (const auto &station : stations)
    {
        lines << QString::number(station.getId()) + " - " + station.getName();
    }
    return lines.join('\n') + "\n\n";
}

QString TransitManager::exportTraversals()
{
    QString content;
    content += buildTraversalText("Recorrido en preorden", tree.preOrder());
    content += buildTraversalText("Recorrido en inorden", tree.inOrder());
    content += buildTraversalText("Recorrido en postorden", tree.postOrder());
    exportTraversalsToFile(content);
    return content;
}

QString TransitManager::buildStationsReport() const
{
    auto stations = tree.inOrder();
    std::sort(stations.begin(), stations.end(), [](const Station &a, const Station &b)
              { return a.getName().localeAwareCompare(b.getName()) < 0; });
    QStringList lines;
    lines << "Estaciones registradas:";
    for (const auto &station : stations)
    {
        lines << QString::number(station.getId()) + " - " + station.getName();
    }
    return lines.join('\n');
}

QString TransitManager::buildRoutesReport() const
{
    auto routes = graph.getConnections();
    QStringList lines;
    lines << "Rutas activas:";
    for (const auto &route : routes)
    {
        lines << QString("%1 ⇄ %2 : %3 minutos").arg(QString::number(route.from), QString::number(route.to), QString::number(route.weight, 'f', 2));
    }
    auto closures = graph.getClosures();
    if (!closures.empty())
    {
        lines << "Tramos cerrados:";
        for (const auto &closure : closures)
        {
            lines << QString("%1 ⇄ %2").arg(QString::number(closure.first), QString::number(closure.second));
        }
    }
    return lines.join('\n');
}

void TransitManager::exportTraversalsToFile(const QString &content)
{
    dataManager.saveTraversal(content);
}

void TransitManager::saveReportContent(const QString &content)
{
    dataManager.saveReport(content);
    dataManager.appendReportLine(QString("%1 Reporte guardado").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm")));
}

QString TransitManager::getStationName(int id) const
{
    Station station;
    if (tree.find(id, station))
    {
        return station.getName();
    }
    return QString();
}

const StationTree &TransitManager::getTree() const
{
    return tree;
}

const GraphNetwork &TransitManager::getGraph() const
{
    return graph;
}

void TransitManager::scaleStationPositions(double scaleX, double scaleY)
{
    if (!std::isfinite(scaleX) || !std::isfinite(scaleY) || scaleX <= 0.0 || scaleY <= 0.0)
    {
        return;
    }
    bool modified = false;
    tree.forEach([&](Station &station) {
        if (!station.hasCoordinates())
        {
            return;
        }
        QPointF pos = station.getPosition();
        station.setPosition(QPointF(pos.x() * scaleX, pos.y() * scaleY));
        modified = true;
    });
    if (!modified)
    {
        return;
    }
    graph.scaleStationPositions(scaleX, scaleY);
    saveData();
}

int TransitManager::getNextAvailableStationId() const
{
    auto stations = tree.inOrder();
    if (stations.empty())
    {
        return 1;
    }
    int maxId = 0;
    for (const auto &station : stations)
    {
        if (station.getId() > maxId)
        {
            maxId = station.getId();
        }
    }
    return maxId + 1;
}

void TransitManager::generateAutomaticRoutesForStation(int stationId, int maxConnections)
{
    Station targetStation;
    if (!tree.find(stationId, targetStation))
    {
        return;
    }
    
    // Solo generar rutas automáticas si la estación tiene coordenadas
    if (!targetStation.hasCoordinates())
    {
        return;
    }
    
    QPointF targetPos = targetStation.getPosition();
    auto allStations = tree.inOrder();
    
    // Crear lista de estaciones cercanas con sus distancias
    struct NearbyStation
    {
        int id;
        double distance;
    };
    std::vector<NearbyStation> nearbyStations;
    
    for (const auto &station : allStations)
    {
        if (station.getId() == stationId)
        {
            continue; // Saltar la misma estación
        }
        
        if (!station.hasCoordinates())
        {
            continue; // Solo conectar con estaciones que tienen coordenadas
        }
        
        QPointF stationPos = station.getPosition();
        
        // Calcular distancia Manhattan
        double dx = std::abs(stationPos.x() - targetPos.x());
        double dy = std::abs(stationPos.y() - targetPos.y());
        double distance = dx + dy;
        
        if (std::isfinite(distance) && distance > 0.0)
        {
            nearbyStations.push_back({station.getId(), distance});
        }
    }
    
    // Ordenar por distancia (más cercanos primero)
    std::sort(nearbyStations.begin(), nearbyStations.end(), 
              [](const NearbyStation &a, const NearbyStation &b) {
                  return a.distance < b.distance;
              });
    
    // Conectar con las estaciones más cercanas (hasta maxConnections)
    int connectionsAdded = 0;
    for (const auto &nearby : nearbyStations)
    {
        if (connectionsAdded >= maxConnections)
        {
            break;
        }
        
        // Verificar si ya existe una conexión
        auto existingRoutes = graph.getConnections();
        bool routeExists = false;
        for (const auto &route : existingRoutes)
        {
            if ((route.from == stationId && route.to == nearby.id) ||
                (route.from == nearby.id && route.to == stationId))
            {
                routeExists = true;
                break;
            }
        }
        
        if (!routeExists)
        {
            // Crear ruta automática con el peso calculado
            if (graph.addConnection(stationId, nearby.id, nearby.distance))
            {
                dataManager.appendReportLine(
                    QString("%1 Ruta automática agregada: %2 ⇄ %3 (%4 minutos)")
                        .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"),
                             QString::number(stationId),
                             QString::number(nearby.id),
                             QString::number(nearby.distance, 'f', 2)));
                connectionsAdded++;
            }
        }
    }
    
    if (connectionsAdded > 0)
    {
        saveData();
    }
}

void TransitManager::regenerateAllAutomaticRoutes(int maxConnectionsPerStation)
{
    auto allStations = tree.inOrder();
    
    for (const auto &station : allStations)
    {
        if (station.hasCoordinates())
        {
            generateAutomaticRoutesForStation(station.getId(), maxConnectionsPerStation);
        }
    }
}
