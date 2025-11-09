#include "GraphNetwork.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <queue>
#include <stack>

GraphNetwork::GraphNetwork()
{
}

bool GraphNetwork::addStation(const Station &station)
{
    if (hasStation(station.getId()))
    {
        return false;
    }
    stationList.push_back(station);
    resizeMatrix();
    rebuildIndices();
    return true;
}

bool GraphNetwork::removeStation(int id)
{
    int index = indexOf(id);
    if (index < 0)
    {
        return false;
    }
    stationList.erase(stationList.begin() + index);
    baseMatrix.erase(baseMatrix.begin() + index);
    matrix.erase(matrix.begin() + index);
    for (auto &row : baseMatrix)
    {
        row.erase(row.begin() + index);
    }
    for (auto &row : matrix)
    {
        row.erase(row.begin() + index);
    }
    rebuildIndices();
    return true;
}

bool GraphNetwork::addConnection(int fromId, int toId, double weight)
{
    int fromIndex = indexOf(fromId);
    int toIndex = indexOf(toId);
    if (fromIndex < 0 || toIndex < 0 || fromIndex == toIndex)
    {
        return false;
    }
    if (fromIndex >= static_cast<int>(baseMatrix.size()) || toIndex >= static_cast<int>(baseMatrix.size()))
    {
        return false;
    }
    baseMatrix[fromIndex][toIndex] = weight;
    baseMatrix[toIndex][fromIndex] = weight;
    matrix[fromIndex][toIndex] = weight;
    matrix[toIndex][fromIndex] = weight;
    return true;
}

bool GraphNetwork::removeConnection(int fromId, int toId)
{
    int fromIndex = indexOf(fromId);
    int toIndex = indexOf(toId);
    if (fromIndex < 0 || toIndex < 0)
    {
        return false;
    }
    double inf = std::numeric_limits<double>::infinity();
    baseMatrix[fromIndex][toIndex] = inf;
    baseMatrix[toIndex][fromIndex] = inf;
    matrix[fromIndex][toIndex] = inf;
    matrix[toIndex][fromIndex] = inf;
    return true;
}

bool GraphNetwork::hasStation(int id) const
{
    return indexById.find(id) != indexById.end();
}

std::vector<Station> GraphNetwork::getStations() const
{
    return stationList;
}

std::vector<GraphEdge> GraphNetwork::getConnections() const
{
    std::vector<GraphEdge> edges;
    for (size_t i = 0; i < baseMatrix.size(); ++i)
    {
        for (size_t j = i + 1; j < baseMatrix.size(); ++j)
        {
            if (std::isfinite(baseMatrix[i][j]))
            {
                edges.push_back({stationList[i].getId(), stationList[j].getId(), baseMatrix[i][j]});
            }
        }
    }
    return edges;
}

std::vector<std::pair<int, int>> GraphNetwork::getClosures() const
{
    return activeClosures;
}

void GraphNetwork::applyClosures(const std::vector<std::pair<int, int>> &closures)
{
    activeClosures = closures;
    matrix = baseMatrix;
    double inf = std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < matrix.size(); ++i)
    {
        for (size_t j = 0; j < matrix.size(); ++j)
        {
            if (i == j)
            {
                matrix[i][j] = 0;
            }
        }
    }
    for (const auto &closure : closures)
    {
        int fromIndex = indexOf(closure.first);
        int toIndex = indexOf(closure.second);
        if (fromIndex >= 0 && toIndex >= 0)
        {
            matrix[fromIndex][toIndex] = inf;
            matrix[toIndex][fromIndex] = inf;
        }
    }
}

std::vector<int> GraphNetwork::bfs(int startId) const
{
    int startIndex = indexOf(startId);
    if (startIndex < 0)
    {
        return {};
    }
    std::vector<int> visitedOrder;
    std::vector<bool> visited(matrix.size(), false);
    std::queue<int> pending;
    visited[startIndex] = true;
    pending.push(startIndex);
    while (!pending.empty())
    {
        int index = pending.front();
        pending.pop();
        visitedOrder.push_back(stationList[index].getId());
        for (size_t neighbor = 0; neighbor < matrix.size(); ++neighbor)
        {
            if (std::isfinite(matrix[index][neighbor]) && !visited[neighbor] && index != static_cast<int>(neighbor))
            {
                visited[neighbor] = true;
                pending.push(static_cast<int>(neighbor));
            }
        }
    }
    return visitedOrder;
}

std::vector<int> GraphNetwork::dfs(int startId) const
{
    int startIndex = indexOf(startId);
    if (startIndex < 0)
    {
        return {};
    }
    std::vector<int> visitedOrder;
    std::vector<bool> visited(matrix.size(), false);
    std::stack<int> pending;
    pending.push(startIndex);
    while (!pending.empty())
    {
        int index = pending.top();
        pending.pop();
        if (visited[index])
        {
            continue;
        }
        visited[index] = true;
        visitedOrder.push_back(stationList[index].getId());
        for (int neighbor = static_cast<int>(matrix.size()) - 1; neighbor >= 0; --neighbor)
        {
            if (std::isfinite(matrix[index][neighbor]) && !visited[neighbor] && index != neighbor)
            {
                pending.push(neighbor);
            }
        }
    }
    return visitedOrder;
}

PathDetail GraphNetwork::dijkstra(int startId, int endId) const
{
    int startIndex = indexOf(startId);
    int endIndex = indexOf(endId);
    if (startIndex < 0 || endIndex < 0)
    {
        return {{}, std::numeric_limits<double>::infinity()};
    }
    size_t size = matrix.size();
    std::vector<double> distances(size, std::numeric_limits<double>::infinity());
    std::vector<int> previous(size, -1);
    distances[startIndex] = 0;
    using Node = std::pair<double, int>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> queue;
    queue.push({0, startIndex});
    while (!queue.empty())
    {
        auto [dist, index] = queue.top();
        queue.pop();
        if (dist > distances[index])
        {
            continue;
        }
        if (index == endIndex)
        {
            break;
        }
        for (size_t neighbor = 0; neighbor < size; ++neighbor)
        {
            double weight = matrix[index][neighbor];
            if (!std::isfinite(weight) || index == static_cast<int>(neighbor))
            {
                continue;
            }
            double tentative = dist + weight;
            if (tentative < distances[neighbor])
            {
                distances[neighbor] = tentative;
                previous[neighbor] = index;
                queue.push({tentative, static_cast<int>(neighbor)});
            }
        }
    }
    if (!std::isfinite(distances[endIndex]))
    {
        return {{}, std::numeric_limits<double>::infinity()};
    }
    std::vector<int> path;
    for (int current = endIndex; current != -1; current = previous[current])
    {
        path.push_back(stationList[current].getId());
    }
    std::reverse(path.begin(), path.end());
    return {path, distances[endIndex]};
}

PathDetail GraphNetwork::floydWarshall(int startId, int endId) const
{
    int startIndex = indexOf(startId);
    int endIndex = indexOf(endId);
    if (startIndex < 0 || endIndex < 0)
    {
        return {{}, std::numeric_limits<double>::infinity()};
    }
    size_t size = matrix.size();
    std::vector<std::vector<double>> dist = matrix;
    std::vector<std::vector<int>> next(size, std::vector<int>(size, -1));
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t j = 0; j < size; ++j)
        {
            if (std::isfinite(dist[i][j]) && i != j)
            {
                next[i][j] = static_cast<int>(j);
            }
        }
        dist[i][i] = 0;
        next[i][i] = static_cast<int>(i);
    }
    for (size_t k = 0; k < size; ++k)
    {
        for (size_t i = 0; i < size; ++i)
        {
            for (size_t j = 0; j < size; ++j)
            {
                if (!std::isfinite(dist[i][k]) || !std::isfinite(dist[k][j]))
                {
                    continue;
                }
                double candidate = dist[i][k] + dist[k][j];
                if (candidate < dist[i][j])
                {
                    dist[i][j] = candidate;
                    next[i][j] = next[i][k];
                }
            }
        }
    }
    if (next[startIndex][endIndex] == -1)
    {
        return {{}, std::numeric_limits<double>::infinity()};
    }
    std::vector<int> path;
    int current = startIndex;
    path.push_back(stationList[current].getId());
    while (current != endIndex)
    {
        current = next[current][endIndex];
        if (current == -1)
        {
            return {{}, std::numeric_limits<double>::infinity()};
        }
        path.push_back(stationList[current].getId());
    }
    return {path, dist[startIndex][endIndex]};
}

TreeDetail GraphNetwork::prim() const
{
    size_t size = matrix.size();
    if (size == 0)
    {
        return {{}, 0};
    }
    std::vector<double> key(size, std::numeric_limits<double>::infinity());
    std::vector<int> parent(size, -1);
    std::vector<bool> inMst(size, false);
    key[0] = 0;
    for (size_t count = 0; count < size - 1; ++count)
    {
        double minValue = std::numeric_limits<double>::infinity();
        int u = -1;
        for (size_t i = 0; i < size; ++i)
        {
            if (!inMst[i] && key[i] < minValue)
            {
                minValue = key[i];
                u = static_cast<int>(i);
            }
        }
        if (u == -1)
        {
            break;
        }
        inMst[u] = true;
        for (size_t v = 0; v < size; ++v)
        {
            double weight = matrix[u][v];
            if (std::isfinite(weight) && !inMst[v] && weight < key[v] && u != static_cast<int>(v))
            {
                key[v] = weight;
                parent[v] = u;
            }
        }
    }
    TreeDetail result;
    result.total = 0;
    for (size_t i = 1; i < size; ++i)
    {
        if (parent[i] != -1 && std::isfinite(matrix[i][parent[i]]))
        {
            result.edges.push_back({stationList[parent[i]].getId(), stationList[i].getId(), matrix[i][parent[i]]});
            result.total += matrix[i][parent[i]];
        }
    }
    return result;
}

TreeDetail GraphNetwork::kruskal() const
{
    size_t size = matrix.size();
    std::vector<GraphEdge> edges;
    for (size_t i = 0; i < size; ++i)
    {
        for (size_t j = i + 1; j < size; ++j)
        {
            if (std::isfinite(matrix[i][j]))
            {
                edges.push_back({stationList[i].getId(), stationList[j].getId(), matrix[i][j]});
            }
        }
    }
    std::sort(edges.begin(), edges.end(), [](const GraphEdge &a, const GraphEdge &b) { return a.weight < b.weight; });
    std::vector<int> parent(size);
    std::vector<int> rank(size, 0);
    std::iota(parent.begin(), parent.end(), 0);
    auto findSet = [&](auto self, int v) -> int
    {
        if (parent[v] == v)
        {
            return v;
        }
        parent[v] = self(self, parent[v]);
        return parent[v];
    };
    auto unionSet = [&](int a, int b)
    {
        int rootA = findSet(findSet, a);
        int rootB = findSet(findSet, b);
        if (rootA == rootB)
        {
            return false;
        }
        if (rank[rootA] < rank[rootB])
        {
            std::swap(rootA, rootB);
        }
        parent[rootB] = rootA;
        if (rank[rootA] == rank[rootB])
        {
            rank[rootA]++;
        }
        return true;
    };
    TreeDetail result;
    result.total = 0;
    for (const auto &edge : edges)
    {
        int fromIndex = indexOf(edge.from);
        int toIndex = indexOf(edge.to);
        if (unionSet(fromIndex, toIndex))
        {
            result.edges.push_back(edge);
            result.total += edge.weight;
        }
    }
    return result;
}

double GraphNetwork::getWeight(int fromId, int toId) const
{
    int fromIndex = indexOf(fromId);
    int toIndex = indexOf(toId);
    if (fromIndex < 0 || toIndex < 0)
    {
        return std::numeric_limits<double>::infinity();
    }
    return matrix[fromIndex][toIndex];
}

void GraphNetwork::clear()
{
    stationList.clear();
    indexById.clear();
    baseMatrix.clear();
    matrix.clear();
    activeClosures.clear();
}

int GraphNetwork::indexOf(int id) const
{
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return -1;
    }
    return it->second;
}

void GraphNetwork::resizeMatrix()
{
    size_t size = stationList.size();
    double inf = std::numeric_limits<double>::infinity();
    baseMatrix.resize(size);
    matrix.resize(size);
    for (size_t i = 0; i < size; ++i)
    {
        baseMatrix[i].resize(size, inf);
        matrix[i].resize(size, inf);
        baseMatrix[i][i] = 0;
        matrix[i][i] = 0;
    }
}

void GraphNetwork::rebuildIndices()
{
    indexById.clear();
    for (size_t i = 0; i < stationList.size(); ++i)
    {
        indexById[stationList[i].getId()] = static_cast<int>(i);
    }
    applyClosures(activeClosures);
}
