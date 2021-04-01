#define WIN32_LEAN_AND_MEAN
#define MAXNAMELEN 32
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<string>
#include <chrono>
#include <WS2tcpip.h>
#include <winsock2.h>
#include <ctype.h>
#include <thread>
#pragma comment (lib, "ws2_32.lib")

using namespace std;

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

// Function to move player to specified positions
int MoveFunc(int x, int y, int clientId, SOCKET out) {
    MoveEvent moving = {
                {{0,0,clientId,Event},Move},
                {x,y},
                {0,0}
    };
    moving.event.head.length = sizeof(moving);
    int sendOK = send(out, (const char*)&moving, moving.event.head.length, 0);
    if (sendOK == SOCKET_ERROR) {
        cout << "Socket error at moving! " << WSAGetLastError() << endl;
    }
    return sendOK;
}

bool isFinished = false;
int x = 0, y = 0;
void DoWork(SOCKET out, int clientId) {
    // Create variables for message holding
    char buf[65536];
    ChangeMsg* msg = (ChangeMsg*)buf;
    NewPlayerPositionMsg* NPPmsg = (NewPlayerPositionMsg*)buf;
    PlayerLeaveMsg* PLmsg = (PlayerLeaveMsg*)buf;
    NewPlayerMsg* NPmsg = (NewPlayerMsg*)buf;

    // Recieve info and write to buf
    recv(out, buf, sizeof(buf), 0);
    // Update position
    MoveFunc(-100, -100, clientId, out);
    recv(out, buf, sizeof(buf), 0);

    // useful info
    cout << "My Stats" << endl;
    cout << "<---------------------------->" << endl;
    cout << "Player ID: " << msg->head.id << endl;
    cout << "SeqNo: " << msg->head.seqNo << endl;
    cout << "Lenght: " << msg->head.length << endl;
    cout << "Type: " << msg->type << endl;
    cout << "X: " << NPPmsg->pos.x << endl;
    cout << "Y: " << NPPmsg->pos.y << endl;
    cout << "<---------------------------->" << endl << endl;


    // add switch case for msghead types and different messages


    // Listen for incomming messages
    while (!isFinished) {
        // Recieve new info and update buf
        recv(out, buf, sizeof(buf), 0);
        msg = (ChangeMsg*)buf;

        // Check message type and print info accordingly
        switch (msg->type)
        {
        case NewPlayer:
            cout << "NewPlayer msg" << endl;
            cout << "Player name: " << NPmsg->name << endl;
            cout << "Player ID: " << NPmsg->msg.head.id << endl << endl;
            break;

        case PlayerLeave:
            cout << "PlayerLeave msg" << endl;
            cout << "Player ID: " << PLmsg->msg.head.id << endl << endl;
            break;

        case NewPlayerPosition:
            cout << "NewPosition msg" << endl;

            cout << "Player ID: " << NPPmsg->msg.head.id << endl;
            cout << "Current pos: x = " << NPPmsg->pos.x << " y = " << NPPmsg->pos.y << endl << endl;

            if (NPmsg->msg.head.id == clientId) {
            // Update position
            x = NPPmsg->pos.x;
            y = NPPmsg->pos.y;
            }
            break;

        default:
            cout << "No change type msg!" << endl;
            break;
        }
    }
}

void main() {
    /// Startup Winsock
    WSADATA data;
    WORD version MAKEWORD(2, 2);
    int wsOK = WSAStartup(version, &data);
    if (wsOK != 0) {
        cout << "Can't start Winsock! " << wsOK;
    }

    /// Create a hint structure for the server
    sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(49153);
    inet_pton(AF_INET, "130.240.40.7", &server.sin_addr);

    /// Socket creation
    SOCKET out = socket(AF_INET, SOCK_STREAM, 0);


    /// Connect to server and creates join message
    connect(out, (const sockaddr*)&server, sizeof(server));
    JoinMsg joining = {
        {0,0,0,Join},
        StaticObject,
        Cone,
        "bill"
    }; 
    joining.head.length = sizeof(joining);

    /// Sends joining package
    int sendOK = send(out, (const char*)&joining, joining.head.length, 0);

    // some variables
    char buf[65536];
    recv(out, buf, sizeof(buf), 0);
    ChangeMsg* msg = (ChangeMsg*)buf;
    int clientId = msg->head.id;

    // Create second thread
    thread listener(DoWork, out, clientId);

    /// Create specified package
    
    while (true) {
        char buff[16];
        scanf_s("%s", buff, sizeof(buff));
        for (int i = 0; i < sizeof(buff); i++)
        {
            buff[i] = tolower(buff[i]);
        }

        // Gives all possible commands
        if (strcmp(buff, "help") == 0) {
            cout << "All possible commands at this time are:"
                << endl << "* moveU" 
                << endl << "* moveD"
                << endl << "* moveL"
                << endl << "* moveR"
                << endl << "* help"
                << endl << "* leave"
                << endl;
        }
        // Move up
        else if (strcmp(buff, "moveu") == 0) {
            MoveFunc(x, y+1, clientId, out);
        }
        // move down
        else if (strcmp(buff, "moved") == 0) {
            MoveFunc(x, y-1, clientId, out);
        }
        // Move left
        else if (strcmp(buff, "movel") == 0) {
            MoveFunc(x-1, y, clientId, out);
        }
        // Move right
        else if (strcmp(buff, "mover") == 0) {
            MoveFunc(x+1, y, clientId, out);
        }
        // Leave
        else if (strcmp(buff, "leave") == 0) {
            // Create leaving message
            LeaveMsg leaving = {
            {0,0,clientId,Leave}
            };
            leaving.head.length = sizeof(leaving);
            int sendOK = send(out, (const char*)&leaving, leaving.head.length, 0);
            // Checks if package got sent OK
            if (sendOK == SOCKET_ERROR) {
                cout << "Socket error at leave! " << WSAGetLastError() << endl;
            }
            break;
        }
        else {
            cout << "Incorrect command input! Try help" << endl;
        }
    }

    /// Close socket and cleanup WSA and let other threads catch up
    isFinished = true;
    listener.join();
    Sleep(500);
    closesocket(out);
    WSACleanup();
}