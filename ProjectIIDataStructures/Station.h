#pragma once

#include <QPointF>
#include <QString>

class Station
{
public:
    Station();
    Station(int stationId, const QString &stationName);
    Station(int stationId, const QString &stationName, const QPointF &position);
    int getId() const;
    QString getName() const;
    void setName(const QString &stationName);
    bool hasCoordinates() const;
    QPointF getPosition() const;
    void setPosition(const QPointF &position);
    void clearPosition();
private:
    int id;
    QString name;
    QPointF coordinates;
    bool coordinatesValid;
};
