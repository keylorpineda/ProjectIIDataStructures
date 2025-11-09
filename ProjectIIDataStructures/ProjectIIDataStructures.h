#pragma once

#include "TransitManager.h"
#include "ui_ProjectIIDataStructures.h"
#include <QCloseEvent>
#include <QGraphicsScene>
#include <QIntValidator>
#include <QtWidgets/QMainWindow>
#include <utility>
#include <vector>

class ProjectIIDataStructures : public QMainWindow
{
    Q_OBJECT

public:
    ProjectIIDataStructures(QWidget *parent = nullptr);
    ~ProjectIIDataStructures();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::ProjectIIDataStructuresClass ui;
    TransitManager manager;
    QIntValidator *stationIdValidator;
    QGraphicsScene *graphScene;
    void setupUiBehavior();
    void refreshStations();
    void refreshRoutes();
    void refreshClosures();
    void refreshCombos();
    void refreshAll();
    void refreshGraphVisualization();
    void displayMessage(const QString &text);
    void displayError(const QString &text);
    int selectedStationId() const;
    std::pair<int, int> selectedRoute() const;
    QString joinStations(const std::vector<int> &ids) const;
};
