#pragma once

#include <QString>

class Station
{
public:
    Station();
    Station(int stationId, const QString &stationName);
    int getId() const;
    QString getName() const;
    void setName(const QString &stationName);
private:
    int id;
    QString name;
};
