#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QTimer>
#include <QListWidget>
#include "Solver.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:

    void on_next_clicked();

    void on_run_toggled(bool checked);

    void step();

    void on_amr_toggled(bool checked);

    void on_ic_box_currentIndexChanged(int index);

    void on_reset_clicked();

private:
    Solver mpm;
    bool AMR;
    Ui::MainWindow *ui;
    QGraphicsScene scene;
    QTimer *timer;
    ParticleGenerator p_gen;
};

#endif // MAINWINDOW_H
