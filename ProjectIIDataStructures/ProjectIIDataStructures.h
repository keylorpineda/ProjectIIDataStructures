#pragma once

#include "InteractiveGraphicsView.h"
#include "TransitManager.h"
#include "ui_ProjectIIDataStructures.h"
#include <QCloseEvent>
#include <QEvent>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QIntValidator>
#include <QPixmap>
#include <QPointF>
#include <QString>
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
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::ProjectIIDataStructuresClass ui;
    TransitManager manager;
    QIntValidator *stationIdValidator;
    QGraphicsScene *graphScene;
    QString mapStorageDirectory;
    QString mapStoredFile;
    QPixmap loadedMapPixmap;
    QGraphicsPixmapItem *mapPixmapItem;
    QRectF mapSceneRect;
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
    void initializeMapStorage();
    void loadPersistedMap();
    bool persistMapPixmap(const QPixmap &pixmap);
    void applyMapPixmap(const QPixmap &pixmap, bool forceFit);
    void clearStoredMap();
    void addMapItemToScene();
    bool mapIsActive() const;
    bool pointWithinMap(const QPointF &point) const;
    void promptAddStationAt(const QPointF &scenePos);
    void handleStationRemovalRequest(int stationId);
};
