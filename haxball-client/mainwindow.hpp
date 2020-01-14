#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "game.h"
#include "settings.h"
#include "clientsocket.hpp"
#include <QListWidgetItem>

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
    void on_exitButton_clicked();
    void on_createButton_clicked();
    void on_settingsButton_clicked();
    void on_refreshButton_clicked();
    void on_joinButton_clicked();

    void enableCreateGameButton();
    void enableJoinGameButton();


    void playerIdReady(QString id);
    void gameNamesReady(QStringList games);

    void on_gamesListWidget_itemSelectionChanged();

private:
    Ui::MainWindow *ui;
    Game* game;
    Settings* settings;
    std::string sendMsg;


    std::shared_ptr<ClientSocket> m_clientsocket;
    QString m_playerId;
    QStringList m_games;
    QString m_playerName;
    QString m_gameName;
    int m_playerNumber;
    QListWidgetItem* m_currentItem = nullptr;

    void setUpListener();
    bool checkCreateGame();
    bool checkJoinGame();

};
#endif // MAINWINDOW_HPP
