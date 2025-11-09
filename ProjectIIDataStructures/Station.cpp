#include "Station.h"

Station::Station() : id(0), coordinates(0.0, 0.0), coordinatesValid(false)
{
}

Station::Station(int stationId, const QString &stationName)
    : id(stationId), name(stationName), coordinates(0.0, 0.0), coordinatesValid(false)
{
}

Station::Station(int stationId, const QString &stationName, const QPointF &position)
    : id(stationId), name(stationName), coordinates(position), coordinatesValid(true)
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

bool Station::hasCoordinates() const
{
    return coordinatesValid;
}

QPointF Station::getPosition() const
{
    return coordinates;
}

void Station::setPosition(const QPointF &position)
{
    coordinates = position;
    coordinatesValid = true;
}

void Station::clearPosition()
{
    coordinates = QPointF(0.0, 0.0);
    coordinatesValid = false;
}
