#include "Station.h"

Station::Station() : id(0)
{
}

Station::Station(int stationId, const QString &stationName) : id(stationId), name(stationName)
{
}

int Station::getId() const
{
    return id;
}

QString Station::getName() const
{
    return name;
}

void Station::setName(const QString &stationName)
{
    name = stationName;
}
