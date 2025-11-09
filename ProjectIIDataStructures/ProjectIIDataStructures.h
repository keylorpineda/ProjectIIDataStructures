#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_ProjectIIDataStructures.h"

class ProjectIIDataStructures : public QMainWindow
{
    Q_OBJECT

public:
    ProjectIIDataStructures(QWidget *parent = nullptr);
    ~ProjectIIDataStructures();

private:
    Ui::ProjectIIDataStructuresClass ui;
};

