#pragma once

#include "Station.h"
#include <unordered_map>
#include <utility>
#include <vector>

struct GraphEdge
{
    int from;
    int to;
    double weight;
};

struct PathDetail
{
    std::vector<int> stations;
    double total;
};

struct TreeDetail
{
    std::vector<GraphEdge> edges;
    double total;
};

class GraphNetwork
{
public:
    GraphNetwork();
    bool addStation(const Station &station);
    bool removeStation(int id);
    bool addConnection(int fromId, int toId, double weight);
    bool removeConnection(int fromId, int toId);
    bool hasStation(int id) const;
    std::vector<Station> getStations() const;
    std::vector<GraphEdge> getConnections() const;
    std::vector<std::pair<int, int>> getClosures() const;
    void applyClosures(const std::vector<std::pair<int, int>> &closures);
    std::vector<int> bfs(int startId) const;
    std::vector<int> dfs(int startId) const;
    PathDetail dijkstra(int startId, int endId) const;
    PathDetail floydWarshall(int startId, int endId) const;
    TreeDetail prim() const;
    TreeDetail kruskal() const;
    double getWeight(int fromId, int toId) const;
    void clear();
private:
    std::vector<Station> stationList;
    std::unordered_map<int, int> indexById;
    std::vector<std::vector<double>> baseMatrix;
    std::vector<std::vector<double>> matrix;
    std::vector<std::pair<int, int>> activeClosures;
    int indexOf(int id) const;
    void resizeMatrix();
    void rebuildIndices();
};
