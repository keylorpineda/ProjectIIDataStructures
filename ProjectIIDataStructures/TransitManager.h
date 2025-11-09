#pragma once

#include "DataManager.h"

class TransitManager
{
public:
    TransitManager();
    void initialize();
    void saveData();
    bool addStation(int id, const QString &name);
    bool removeStation(int id);
    bool addRoute(int fromId, int toId, double time);
    bool removeRoute(int fromId, int toId);
    std::vector<Station> getStations() const;
    std::vector<GraphEdge> getRoutes() const;
    std::vector<std::pair<int, int>> getClosures() const;
    void reloadClosures();
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
private:
    StationTree tree;
    GraphNetwork graph;
    DataManager dataManager;
};
