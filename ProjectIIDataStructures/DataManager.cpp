#include "DataManager.h"
#include <QDir>
#include <QFile>
#include <QPointF>
#include <QTextStream>

DataManager::DataManager()
{
    setBasePath(QDir::currentPath());
}

void DataManager::setBasePath(const QString &path)
{
    QDir dir(path);
    basePath = dir.absolutePath();
    stationsFile = dir.filePath("estaciones.txt");
    routesFile = dir.filePath("rutas.txt");
    closuresFile = dir.filePath("cierres.txt");
    reportsFile = dir.filePath("reportes.txt");
    traversalFile = dir.filePath("recorridos_rutas.txt");
    ensureFiles();
}

void DataManager::load(StationTree &tree, GraphNetwork &graph) const
{
    tree.clear();
    graph.clear();
    QFile stationData(stationsFile);
    if (stationData.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&stationData);
        while (!stream.atEnd())
        {
            QString line = stream.readLine().trimmed();
            if (line.isEmpty())
            {
                continue;
            }
            QStringList parts = line.split(';');
            if (parts.size() < 2)
            {
                continue;
            }
            bool ok = false;
            int id = parts[0].toInt(&ok);
            if (!ok)
            {
                continue;
            }
            QString name = parts[1];
            Station station(id, name);
            if (parts.size() >= 4)
            {
                bool okX = false;
                bool okY = false;
                double x = parts[2].toDouble(&okX);
                double y = parts[3].toDouble(&okY);
                if (okX && okY)
                {
                    station.setPosition(QPointF(x, y));
                }
            }
            tree.insert(station);
            graph.addStation(station);
        }
        stationData.close();
    }
    QFile routesData(routesFile);
    if (routesData.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&routesData);
        while (!stream.atEnd())
        {
            QString line = stream.readLine().trimmed();
            if (line.isEmpty())
            {
                continue;
            }
            QStringList parts = line.split(';');
            if (parts.size() < 3)
            {
                continue;
            }
            bool okFrom = false;
            bool okTo = false;
            bool okWeight = false;
            int fromId = parts[0].toInt(&okFrom);
            int toId = parts[1].toInt(&okTo);
            double weight = parts[2].toDouble(&okWeight);
            if (!okFrom || !okTo || !okWeight)
            {
                continue;
            }
            graph.addConnection(fromId, toId, weight);
        }
        routesData.close();
    }
    auto closures = loadClosures();
    if (!closures.empty())
    {
        graph.applyClosures(closures);
    }
    else
    {
        graph.applyClosures({});
    }
}

void DataManager::save(const StationTree &tree, const GraphNetwork &graph) const
{
    QFile stationData(stationsFile);
    if (stationData.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&stationData);
        auto stations = tree.inOrder();
        for (const auto &station : stations)
        {
            stream << station.getId() << ";" << station.getName();
            if (station.hasCoordinates())
            {
                QPointF pos = station.getPosition();
                stream << ";" << QString::number(pos.x(), 'f', 4) << ";" << QString::number(pos.y(), 'f', 4);
            }
            stream << "\n";
        }
        stationData.close();
    }
    QFile routesData(routesFile);
    if (routesData.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&routesData);
        auto edges = graph.getConnections();
        for (const auto &edge : edges)
        {
            stream << edge.from << ";" << edge.to << ";" << edge.weight << "\n";
        }
        routesData.close();
    }
}

std::vector<std::pair<int, int>> DataManager::loadClosures() const
{
    std::vector<std::pair<int, int>> closures;
    QFile closureData(closuresFile);
    if (closureData.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream stream(&closureData);
        while (!stream.atEnd())
        {
            QString line = stream.readLine().trimmed();
            if (line.isEmpty())
            {
                continue;
            }
            QStringList parts = line.split(';');
            if (parts.size() < 2)
            {
                continue;
            }
            bool okFrom = false;
            bool okTo = false;
            int fromId = parts[0].toInt(&okFrom);
            int toId = parts[1].toInt(&okTo);
            if (!okFrom || !okTo)
            {
                continue;
            }
            closures.emplace_back(fromId, toId);
        }
        closureData.close();
    }
    return closures;
}

void DataManager::saveReport(const QString &content) const
{
    QFile reportData(reportsFile);
    if (reportData.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&reportData);
        stream << content;
        reportData.close();
    }
}

void DataManager::appendReportLine(const QString &line) const
{
    QFile reportData(reportsFile);
    if (reportData.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&reportData);
        stream << line << "\n";
        reportData.close();
    }
}

void DataManager::saveTraversal(const QString &content) const
{
    QFile traversalData(traversalFile);
    if (traversalData.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream stream(&traversalData);
        stream << content;
        traversalData.close();
    }
}

QString DataManager::getBasePath() const
{
    return basePath;
}

void DataManager::ensureFiles() const
{
    QFile stationData(stationsFile);
    if (!stationData.exists())
    {
        stationData.open(QIODevice::WriteOnly);
        stationData.close();
    }
    QFile routesData(routesFile);
    if (!routesData.exists())
    {
        routesData.open(QIODevice::WriteOnly);
        routesData.close();
    }
    QFile reportData(reportsFile);
    if (!reportData.exists())
    {
        reportData.open(QIODevice::WriteOnly);
        reportData.close();
    }
    QFile closureData(closuresFile);
    if (!closureData.exists())
    {
        closureData.open(QIODevice::WriteOnly);
        closureData.close();
    }
    QFile traversalData(traversalFile);
    if (!traversalData.exists())
    {
        traversalData.open(QIODevice::WriteOnly);
        traversalData.close();
    }
}
