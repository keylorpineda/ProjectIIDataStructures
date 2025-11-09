#pragma once

#include "DataManager.h"
#include <QPointF>
#include <optional>

class TransitManager
{
public:
    TransitManager();
    void initialize();
    void saveData();
    bool addStation(int id, const QString &name, const std::optional<QPointF> &position = std::nullopt);
    bool removeStation(int id);
    bool addRoute(int fromId, int toId, const std::optional<double> &time = std::nullopt);
    bool removeRoute(int fromId, int toId);
    std::vector<Station> getStations() const;
    std::vector<GraphEdge> getRoutes() const;
    std::vector<std::pair<int, int>> getClosures() const;
    void reloadClosures();
    QString dataDirectory() const;
    std::vector<int> runBfs(int startId);
    std::vector<int> runDfs(int startId);
    PathDetail runDijkstra(int startId, int endId);
    PathDetail runFloyd(int startId, int endId);
    TreeDetail runPrim();
    TreeDetail runKruskal();
    QString buildTraversalText(const QString &title, const std::vector<Station> &stations) const;
    QString exportTraversals();
    QString buildStationsReport() const;
    QString buildRoutesReport() const;
    void exportTraversalsToFile(const QString &content);
    void saveReportContent(const QString &content);
    QString getStationName(int id) const;
    const StationTree &getTree() const;
    const GraphNetwork &getGraph() const;
    std::optional<double> calculateRouteWeightFromCoordinates(int fromId, int toId) const;
    void scaleStationPositions(double scaleX, double scaleY);
private:
    StationTree tree;
    GraphNetwork graph;
    DataManager dataManager;
};
