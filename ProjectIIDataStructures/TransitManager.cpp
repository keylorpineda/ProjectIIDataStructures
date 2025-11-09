#include "TransitManager.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QStringList>
#include <algorithm>

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

bool TransitManager::addStation(int id, const QString &name)
{
    QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty())
    {
        return false;
    }
    Station station(id, trimmedName);
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

bool TransitManager::addRoute(int fromId, int toId, double time)
{
    if (fromId == toId || time <= 0)
    {
        return false;
    }
    if (!graph.addConnection(fromId, toId, time))
    {
        return false;
    }
    dataManager.appendReportLine(QString("%1 Ruta agregada: %2 ⇄ %3 (%4 minutos)").arg(QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm"), QString::number(fromId), QString::number(toId), QString::number(time, 'f', 2)));
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
