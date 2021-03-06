#include "game.hpp"
#include "ui_game.h"
#include "player.hpp"
#include "clientsocket.hpp"

#include <iostream>
#include <vector>
#include <utility>

#include <QApplication>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QDebug>
#include <QPen>
#include <QTimer>
#include <QObject>
#include <QStringList>
#include <QtMath>


#define UNUSED(expr) do { (void)(expr); } while (0)

Game::Game(std::shared_ptr<ClientSocket> clientsocket, int playerId, int gameId, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Game),
    m_id(gameId),
    m_clientsocket(clientsocket)
{
    m_me = std::make_shared<Player>(0, 0, 0);
    m_me->setId(playerId);

    m_ball = std::make_shared<Ball>(0, 0);

    ui->setupUi(this);

    setUp();
}

Game::~Game()
{
    delete ui;
}

QGraphicsScene *Game::getScene() const
{
    return scene;
}


void Game::on_exit_button_clicked()
{
    QApplication::exit();
}

void Game::coordsRead(QStringList coords)
{
    qDebug() << "[coordsReadReady]: Sa servera je stigla poruka: " << coords;

    if(coords.size() != 0){
        qreal xBall = coords.takeFirst().trimmed().toDouble();
        qreal yBall = coords.takeFirst().trimmed().toDouble();


        m_ball->setX(xBall);
        m_ball->setY(yBall);
    }
    qDebug() << "[coordsRead]: Azurirana je lopta na poziciju (" << m_ball->x() <<", " << m_ball->y() <<")";


    if(coords.size() != 0){

        for(QStringList::iterator iter = coords.begin(); iter != coords.end(); iter += 4){
            int playerId = iter->toInt();

            qreal x = (iter + 1)->toDouble();
            qreal y = (iter + 2)->toDouble();
            int team = (iter + 3)->toInt();

            auto player_it = m_players.find(playerId);
            if(player_it != m_players.end()){
                (*player_it)->setX(x);
                (*player_it)->setY(y);
                qDebug() << "[coordsRead]: Azuriran je postojeci igrac " << playerId << " na poziciju (" << x <<", " << y <<")";
            }
            else{
                std::shared_ptr<Player> player = std::make_shared<Player>(0, 0, team);
                player->setX(x);
                player->setY(y);
                scene->addItem(player.get());
                m_players.insert(playerId, player);

                qDebug() << "[coordsRead]: Dodat je novi igrac " << playerId << " na poziciju (" << x <<", " << y <<") za tim " << team;
            }
            if(!isSetTeam){
                if(m_players.size() % 2 == 0){
                    m_me->setBrush(Qt::black);
                  }
                else{
                    m_me->setBrush(Qt::red);
                  }
                isSetTeam = true;
              }
        }
    }
    if(!isTimerStarted){
        startTimer(1000/60);
        isTimerStarted = true;
    }

}

void Game::coordsWrite()
{
    QByteArray serverRequest;
    const QString protocol = "coords";

    serverRequest.append(protocol + " ")
                 .append(QString::number(m_me->getId()) + " ")
                 .append(QString::number(m_me->x()) + " ")
                 .append(QString::number(m_me->y()) + "\n");

    m_clientsocket->getSocket()->write(serverRequest);
    m_clientsocket->getSocket()->flush();

    qDebug() << "[coordsWrite]: Serveru je poslat zahtev: " << QString(serverRequest);

}

void Game::ballCollisionWrite()
{
    QByteArray serverRequest;
    const QString protocol = "collision";

    serverRequest.append(protocol + " ")
                 .append(QString::number(m_id) + " ")
                 .append(QString::number(m_me->x()) + " ")
                 .append(QString::number(m_me->y()) + " ")
                 .append(QString::number(m_me->getSpeedX()) + " ")
                 .append(QString::number(m_me->getSpeedY()) + " ");

    int isSpace = pressedKeys.contains(Qt::Key_Space) ? 1 : 0;
    serverRequest.append(QString::number(isSpace));

    m_clientsocket->getSocket()->write(serverRequest);
    m_clientsocket->getSocket()->flush();

    qDebug() << "[ballCoords]: Serveru je poslat zahtev: " << QString(serverRequest);

}


void Game::checkGoal() {
    ui->showResult->setText(m_clientsocket->getResult());
    if(m_ball->x() < -20 || m_ball->x() > 980) {
       emit onGoal();
       m_ball->setX(480);
       m_ball->setY(230);
    }
}

void Game::goalWrite() {
    QByteArray serverRequest;
    const QString protocol = "goal";

    if(m_ball->x() < -20) {
        serverRequest.append(protocol + " Crveni tim " + " ");
    }
    // m_ball.x() < 20 za loptu(desni gol)
    else if(m_ball->x() > 980) {
        serverRequest.append(protocol + " Plavi tim " + " ");
    }
     m_clientsocket->getSocket()->write(serverRequest);
     m_clientsocket->getSocket()->flush();

    qDebug() << "[goalWrite]: Serveru je poslat zahtev: " << QString(serverRequest);

}

// Metoda setUpListener registruje signale i njima odgovarajuce slotove i inicijalizuje potrebne komponente.
void Game::setUp()
{
    connect(m_clientsocket.get(), SIGNAL(coords(QStringList)), this, SLOT(coordsRead(QStringList)));
    connect(this, SIGNAL(playerAction()), this, SLOT(coordsWrite()));
    connect(this, SIGNAL(onGoal()), this, SLOT(goalWrite()));
    connect(this, SIGNAL(ballCollisionDetected()), this, SLOT(ballCollisionWrite()));

    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    scene = new QGraphicsScene();
    scene = drawField();
    ui->view->setScene(scene);
    setWindowState(Qt::WindowFullScreen);
    installEventFilter(this);
    scene->addItem(m_me.get());
    scene->addItem(m_ball.get());

    emit coordsWrite();
}

void Game::setId(int value)
{
    m_id = value;
}

int Game::getId() const
{
    return m_id;
}

std::shared_ptr<Player> Game::getMe() const
{
    return m_me;
}

void Game::setSocket(std::shared_ptr<ClientSocket> sock)
{
    m_clientsocket = sock;
}

bool Game::eventFilter(QObject *obj, QEvent *event)
{
    UNUSED(obj);

    if(event->type()==QEvent::KeyPress) {
        pressedKeys += (static_cast<QKeyEvent*>(event))->key();
        emit playerAction();
    }
    else if(event->type()==QEvent::KeyRelease){
        pressedKeys -= (static_cast<QKeyEvent*>(event))->key();
        emit playerAction();
    }

    return false;
}


void Game::timerEvent(QTimerEvent *event)
{
    UNUSED(event);

    if(pressedKeys.contains(Qt::Key_Left)){
        m_me->accelerateX(-Player::ACCELERATION);
    }
    if(pressedKeys.contains(Qt::Key_Right)){
        m_me->accelerateX(Player::ACCELERATION);
    }
    if(pressedKeys.contains(Qt::Key_Up)){
        m_me->accelerateY(-Player::ACCELERATION);
    }
    if(pressedKeys.contains(Qt::Key_Down)){
        m_me->accelerateY(Player::ACCELERATION);
    }
    if(pressedKeys.contains(Qt::Key_Space)){
        m_me->setPen(QPen(Qt::yellow, 5, Qt::SolidLine));
    }
    else{
        m_me->setPen(QPen(Qt::white, 5, Qt::SolidLine));
    }

    emit playerAction();


    if(m_me.get()->collidesWithItem(m_ball.get())){

        emit ballCollisionDetected();
    }

    m_me->moveBy(m_me->getSpeedX(), m_me->getSpeedY());
    m_me->slow(Player::SLOWING);

    checkGoal();

}

QGraphicsScene* Game::drawField()
{
    QGraphicsScene* scene = getScene();
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(5);

    // the stadium and goals
    scene->addRect(-50, 175, 50, 150, pen);
    scene->addRect(1000, 175, 50, 150, pen);
    pen.setColor(Qt::white);
    scene->addRect(0, 0, 1000, 500, pen);
    scene->addLine(500, 0, 500, 500,pen);

    // center
    scene->addEllipse(400, 150, 200, 200,pen);
    // goalpost
    pen.setColor(Qt::black);
    pen.setWidth(2);
    scene->addEllipse(-10, 165 , 20, 20,pen, QBrush(Qt::white));
    scene->addEllipse(-10, 315, 20, 20,pen, QBrush(Qt::white));
    scene->addEllipse(990, 165, 20, 20,pen, QBrush(Qt::white));
    scene->addEllipse(990, 315, 20, 20,pen, QBrush(Qt::white));
    return scene;
}
