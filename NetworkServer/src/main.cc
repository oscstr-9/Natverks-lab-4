#define WIN32_LEAN_AND_MEAN
#define MAXNAMELEN 32
#define START_POS Coordinate{ -100, -100 }
#if _WIN32
    #define PRINT_SOCKET_ERROR() std::cout << GetLastError() << std::endl;
    #define CLOSE_SOCKET(socket) closesocket(socket);

#else
    #define PRINT_SOCKET_ERROR()
    #define CLOSE_SOCKET(socket) close(socket);

#endif


#include <iostream>
#include <WS2tcpip.h>
#include <winsock2.h>
#include <unordered_map>
#pragma comment (lib, "ws2_32.lib")

unsigned int globalID = 0;

enum ObjectDesc {
    Human,
    NonHuman,
    Vehicle,
    StaticObject
};

enum ObjectForm {
    Cube,
    Sphere,
    Pyramid,
    Cone
};

struct Coordinate {
    int x;
    int y;
};

enum MsgType {
    Join,           //Client joining game at server
    Leave,          //Client leaving game
    Change,         //Information to clients
    Event,          //Information from clients to server
    TextMessage     //Send text messages to one or all
};

//MESSAGE HEAD, Included first in all messages   
struct MsgHead {
    unsigned int length;     //Total length for whole message   
    unsigned int seqNo;      //Sequence number
    unsigned int id;         //Client ID or 0;
    MsgType type;            //Type of message
};

//JOIN MESSAGE (CLIENT->SERVER)
struct JoinMsg {
    MsgHead head;
    ObjectDesc desc;
    ObjectForm form;
    char name[MAXNAMELEN];   //null terminated!,or empty
};

//LEAVE MESSAGE (CLIENT->SERVER)
struct LeaveMsg {
    MsgHead head;
};

//CHANGE MESSAGE (SERVER->CLIENT)
enum ChangeType {
    NewPlayer,
    PlayerLeave,
    NewPlayerPosition
};

//Included first in all Change messages
struct ChangeMsg {
    MsgHead head;
    ChangeType type;
};

struct NewPlayerMsg {
    ChangeMsg msg;          //Change message header with new client id
    ObjectDesc desc;
    ObjectForm form;
    char name[MAXNAMELEN];  //nullterminated!,or empty
};

struct PlayerLeaveMsg {
    ChangeMsg msg;          //Change message header with new client id
};

struct NewPlayerPositionMsg {
    ChangeMsg msg;          //Change message header
    Coordinate pos;         //New object position
    Coordinate dir;         //New object direction
};

//EVENT MESSAGE (CLIENT->SERVER)
enum EventType { Move };

//Included first in all Event messages
struct EventMsg {
    MsgHead head;
    EventType type;
};

//Variantions of EventMsg
struct MoveEvent {
    EventMsg event;
    Coordinate pos;         //New object position
    Coordinate dir;         //New object direction
};

//TEXT MESSAGE
struct TextMessageMsg {
    MsgHead head;
    char text[1];   //NULL-terminerad array of chars.
};

struct Player
{
    Coordinate pos;
    unsigned int ID = 0;
    ObjectDesc objDesc;
    ObjectForm objForm;
    SOCKET socket;
    char name[MAXNAMELEN];
};

std::unordered_map<int, Player> playerList;


/**
Handles join msgs
@param The incomming message as a JoinMsg
@param The new player
*/
void joinMsgRecieved(JoinMsg* msg, Player& newPlayer) {
    std::cout << "player joined" << std::endl;

    Coordinate spawnPos = START_POS;
    int retries = 1;
    for (int i = 0; i < retries; i++)
    {
        for (auto& player : playerList) {
            if (player.second.pos.x == spawnPos.x && player.second.pos.y == spawnPos.y)
            {
                spawnPos.x++;
                retries++;
                break;
            }
        }
    }

    newPlayer.pos = spawnPos;
    newPlayer.objDesc = msg->desc;
    newPlayer.objForm = msg->form;
    for (int i = 0; i < MAXNAMELEN; i++) {
        newPlayer.name[i] = msg->name[i];
    }

    // NewPlayerMsg that will be sent to everyone
    NewPlayerMsg NPMsg{
        ChangeMsg{MsgHead{sizeof(NewPlayerMsg), 0, newPlayer.ID, Change}, NewPlayer},
        newPlayer.objDesc,
        newPlayer.objForm,
        *msg->name
    };

    // NewPlayerPositionMsg that will be sent to everyone
    NewPlayerPositionMsg NPPMsg{
        ChangeMsg{MsgHead{sizeof(NewPlayerPositionMsg), 0, newPlayer.ID, Change}, NewPlayerPosition},
        newPlayer.pos,
        START_POS
    };

    for (auto& player : playerList) {
        // NewPlayerMsg send
        int sendOk = send(player.second.socket, (const char*)&NPMsg, NPMsg.msg.head.length, 0);
        if (sendOk == SOCKET_ERROR) {
            std::cout << "Could not send new player msg!" << std::endl;
            return;
        }

        // NewPlayerPosition send
        sendOk = send(player.second.socket, (const char*)&NPPMsg, NPPMsg.msg.head.length, 0);
        if (sendOk == SOCKET_ERROR) {
            std::cout << "Could not send new player msg!" << std::endl;
            return;
        }
    }

    // Update the new player on all of the already connected players
    for (auto& player : playerList) {

        std::cout << player.first << std::endl;
        if (player.first == newPlayer.ID) continue;
        std::cout << "sent " << player.first << " to newPlayer: " << newPlayer.ID << std::endl;

        // NewPlayerMsg that will be sent to the new player
        NewPlayerMsg NPMsg{
            ChangeMsg{MsgHead{sizeof(NewPlayerMsg), 0, player.second.ID, Change}, NewPlayer},
            player.second.objDesc,
            player.second.objForm,
            *player.second.name
        };

        //NewPlayerPositionMsg that will be sent to the new player
        NewPlayerPositionMsg NPPMsg{
            ChangeMsg{MsgHead{sizeof(NewPlayerPositionMsg), 0, player.second.ID, Change}, NewPlayerPosition},
            player.second.pos,
            START_POS
        };

        // NewPlayerMsg send
        int sendOk = send(newPlayer.socket, (const char*)&NPMsg, NPMsg.msg.head.length, 0);
        if (sendOk == SOCKET_ERROR) {
            std::cout << "Could not send new player msg!" << std::endl;
            return;
        }

        // NewPlayerPosition send
        sendOk = send(newPlayer.socket, (const char*)&NPPMsg, NPPMsg.msg.head.length, 0);
        if (sendOk == SOCKET_ERROR) {
            std::cout << "Could not send new player msg!" << std::endl;
            return;
        }
    }
    
}

/**
Handles leave msgs
@param The incomming message as a LeaveMsg
@param The player that left
@param The set of all connected sockets/players
@param A reference to vector with all players that should be removed
*/void leaveMsgRecieved(LeaveMsg* msg, Player &leavePlayer, fd_set &master, std::vector<Player>& playersToRemove) {
    std::cout << "player left" << std::endl;

    CLOSE_SOCKET(leavePlayer.socket);
    FD_CLR(leavePlayer.socket, &master);
    playersToRemove.push_back(leavePlayer);
    PlayerLeaveMsg PLMsg{ ChangeMsg{ MsgHead{sizeof(PlayerLeaveMsg), 0, leavePlayer.ID, Change}, PlayerLeave } };

    for (auto& player : playerList) {
        // NewPlayerMsg send
        int sendOk = send(player.second.socket, (const char*)&PLMsg, PLMsg.msg.head.length, 0);
        if (sendOk == SOCKET_ERROR) {
            std::cout << "Could not send new player msg!" << std::endl;
            return;
        }
    }
}

/**
Handles event msgs
@param The incomming message as an EventMsg
@param The player that moved
*/
void eventMsgRecieved(EventMsg* msg, Player &movePlayer) {

    if (msg->type != Move)
        return;

    MoveEvent* moveEvent = (MoveEvent*)msg;
    for (auto& player : playerList) {
        if (player.second.pos.x == moveEvent->pos.x && player.second.pos.y == moveEvent->pos.y)
        {
            return;
        }
    }
    movePlayer.pos = moveEvent->pos;

    // NewPlayerPositionMsg that will be sent to everyone
    NewPlayerPositionMsg NPPMsg{
        ChangeMsg{MsgHead{sizeof(NewPlayerPositionMsg), 0, movePlayer.ID, Change}, NewPlayerPosition},
        moveEvent->pos,
        START_POS
    };

    for (auto& player : playerList) {
        // NewPlayerMsg send
        int sendOk = send(player.second.socket, (const char*)&NPPMsg, NPPMsg.msg.head.length, 0);
        if (sendOk == SOCKET_ERROR) {
            std::cout << "Could not send new player msg!" << std::endl;
            return;
        }
    }
}

/**
Checks the type of the incomming msg and calls the corresponding function
@param The incomming message as a MsgHead
@param The player that did a thing
@param The set of all connected sockets/players
@param A reference to vector with all players that should be removed
*/
void checkPlayerMsgs(MsgHead* msg, Player &player, fd_set &master, std::vector<Player> &playersToRemove) {

    switch (msg->type) {
    case Join:
        joinMsgRecieved((JoinMsg*)msg, player);
        break;
    case Leave:
        leaveMsgRecieved((LeaveMsg*)msg, player, master, playersToRemove);
        break;
    case Event:
        eventMsgRecieved((EventMsg*)msg, player);
        break;
    }
}

/**
Main function. sets up sockets and addresses to be used throughout the entire program for both sending and recieving
*/
void main(){
    // starup Winsock
    #if _WIN32
        WSADATA data;
        WORD version MAKEWORD(2, 2);
        int wsOK = WSAStartup(version, &data);
        if (wsOK != 0) {
            std::cout << "Can't start Winsock! " << wsOK;
            return;
        }
    #endif

    char buf[65536];


    sockaddr_in socketAddr;
    memset(&socketAddr, 0, sizeof(socketAddr));
    socketAddr.sin_family = AF_INET;
    socketAddr.sin_port = htons(9002);
    socketAddr.sin_addr.s_addr = INADDR_ANY;
    SOCKET ListeningSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(ListeningSocket, (sockaddr*)&socketAddr, sizeof(socketAddr)) == -1) {
        PRINT_SOCKET_ERROR();
    };

    if (listen(ListeningSocket, 0) == -1) {
        PRINT_SOCKET_ERROR();
    };

    // Create the master file descriptor set and zero it
    fd_set master;
    FD_ZERO(&master);

    FD_SET(ListeningSocket, &master);

    std::vector<Player> playersToRemove;
    //Send package to everyone
    while (true) {
        fd_set copy = master;

        if (select(0, &copy, nullptr, nullptr, nullptr) == -1) {
            PRINT_SOCKET_ERROR();
            return;
        }

        if (FD_ISSET(ListeningSocket, &copy)) {
            SOCKET newConnection = accept(ListeningSocket, NULL, NULL);
            if (newConnection == -1) {
                PRINT_SOCKET_ERROR();
            }
            FD_SET(newConnection, &master);
            Player newPlayer;
            newPlayer.socket = newConnection;

            // finds an free id for the new player
            newPlayer.ID = globalID++%8;
            int retries = 1;
            for (int i = 0; i < retries; i++)
            {
                for (auto& player : playerList) {
                    if (player.second.ID == newPlayer.ID)
                    {
                        newPlayer.ID++;
                        retries++;
                        break;
                    }
                }
            }
            playerList.insert({ newPlayer.ID, newPlayer });
        }

        if (playerList.size() > 0) {
            for (auto& player : playerList) {
                if (FD_ISSET(player.second.socket, &copy)) {
                    recv(player.second.socket, buf, sizeof(buf), 0);
                    checkPlayerMsgs((MsgHead*)buf, player.second, master, playersToRemove);
                }
            }
        }
        if (playersToRemove.size() > 0) {
            for (int i = 0; i < playersToRemove.size(); i++)
            {
                playerList.erase(playersToRemove[i].ID);
            }
        }
    }

    CLOSE_SOCKET(ListeningSocket);
    #if _WIN32
        WSACleanup();
    #endif
}