#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_startButton_clicked();

  void on_stopButton_clicked();

  void on_restartButton_clicked();

  void on_exitButton_clicked();

private:
  Ui::MainWindow *ui;
};
#endif // MAINWINDOW_HPP
