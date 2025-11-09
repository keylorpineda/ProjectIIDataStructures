#pragma once

#include "GraphNetwork.h"
#include "StationTree.h"
#include <QString>
#include <vector>

class DataManager
{
public:
    DataManager();
    void setBasePath(const QString &path);
    void load(StationTree &tree, GraphNetwork &graph) const;
    void save(const StationTree &tree, const GraphNetwork &graph) const;
    std::vector<std::pair<int, int>> loadClosures() const;
    void saveReport(const QString &content) const;
    void appendReportLine(const QString &line) const;
    void saveTraversal(const QString &content) const;
private:
    QString basePath;
    QString stationsFile;
    QString routesFile;
    QString closuresFile;
    QString reportsFile;
    QString traversalFile;
    void ensureFiles() const;
};
