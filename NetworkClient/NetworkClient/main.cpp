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
#include <unordered_map>
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

// Info for each pixel to be sent
struct PixelInfo {
    BYTE x;
    BYTE y;
    BYTE color;
};

//all players currently connected
std::unordered_map<int, int[2]> playerInfo;

/// Holds all info to be sent via network
struct SendInfo {
    short amountOfPixels;
    PixelInfo pixels[5120];
}pack;

/**
Sends the positions of all clients to the GUI 
@param the socket that connects to the java GUI
*/
void SendPosToGui(SOCKET javaSocket) {
    // Bind the ip address and port to a socket
    sockaddr_in6 hint;
    memset(&hint, 0, sizeof(hint));
    hint.sin6_family = AF_INET6;
    hint.sin6_port = htons(54000);
    inet_pton(AF_INET6, "::1", &hint.sin6_addr);

    char intToBytePosOffset = 100;
    SendInfo info;
    info.amountOfPixels = playerInfo.size();
    int j = 0;
    for (auto i : playerInfo) {
        info.pixels[j] = PixelInfo{ (unsigned char)(intToBytePosOffset + i.second[0]), (unsigned char)(intToBytePosOffset + i.second[1]) ,(unsigned char)i.first };
        j++;
    }
    int sendOk = sendto(javaSocket, (const char*)&info, sizeof(PixelInfo)*info.amountOfPixels + 2, 0, (sockaddr*)&hint, sizeof(hint));
    if (sendOk == SOCKET_ERROR) {
        std::cout << "Could not send pos msg!" << endl;
        return;
    }
}

/**
Sends a leave message to the server
@param the socket that connects to the java GUI
@param the socket that connects to the server
@param this clients ID
*/
void SendLeaveToServer(SOCKET javaSocket, SOCKET gameSocket, unsigned int clientId) {
    // Bind the ip address and port to a socket
    sockaddr_in6 hint;
    hint.sin6_family = AF_INET6;
    hint.sin6_port = htons(54000);
    hint.sin6_addr = in6addr_any; // Could also use inet_pton ....

    LeaveMsg leaveMsg{ MsgHead{sizeof(LeaveMsg), 0, clientId, Leave}};
    int sendOk = sendto(gameSocket, (const char*)&leaveMsg, sizeof(leaveMsg), 0, (sockaddr*)&hint, sizeof(hint));
}


/**
Function to move player to specified positions
@param this clients x pos
@param this clients y pos
@param this clients ID
@param the socket that connects to the server
*/
int MoveFunc(int x, int y, int clientId, SOCKET gameSocket) {
    std::cout << x << " " << y << std::endl;
    MoveEvent moving = {
                {{0,0,clientId,Event},Move},
                {x,y},
                {0,0}
    };
    moving.event.head.length = sizeof(moving);
    int sendOK = send(gameSocket, (const char*)&moving, moving.event.head.length, 0);
    if (sendOK == SOCKET_ERROR) {
        cout << "Socket error at moving! " << WSAGetLastError() << endl;
    }
    return sendOK;
}

int x = 0, y = 0;

/**
Recieves and decrypts the incomming messages from the server
@param the socket that connects to the server
@param the socket that connects to the java GUI
@param this clients ID
*/
void DoWork(SOCKET gameSocket, SOCKET javaSocket, int thisClientId) {
    // Create variables for message holding
        char buf[65536];

    // Recieve info and write to buf
     int msgSize = recv(gameSocket, buf, sizeof(buf), 0);
     
     int bufOffset = 0;

    while (true) {
        ChangeMsg* msg = (ChangeMsg*)(buf + bufOffset);
        MsgHead* msghead = (MsgHead*)(buf + bufOffset);
        NewPlayerPositionMsg* NPPmsg = (NewPlayerPositionMsg*)(buf + bufOffset);
        PlayerLeaveMsg* PLmsg = (PlayerLeaveMsg*)(buf + bufOffset);
        NewPlayerMsg* NPmsg = (NewPlayerMsg*)(buf + bufOffset);

        // useful info
        /*cout << "My Stats" << endl;
        cout << "<---------------------------->" << endl;
        cout << "Player ID: " << msg->head.id << endl;
        cout << "SeqNo: " << msg->head.seqNo << endl;
        cout << "Length: " << msg->head.length << endl;
        cout << "Type: " << msg->type << endl;
        cout << "X: " << NPPmsg->pos.x << endl;
        cout << "Y: " << NPPmsg->pos.y << endl;
        cout << "<---------------------------->" << endl << endl;*/

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
            playerInfo.erase(PLmsg->msg.head.id);
            break;

        case NewPlayerPosition:
            cout << "NewPosition msg" << endl;

            cout << "Player ID: " << NPPmsg->msg.head.id << endl;
            cout << "Current pos: x = " << NPPmsg->pos.x << " y = " << NPPmsg->pos.y << endl << endl;

            playerInfo[NPPmsg->msg.head.id][0] = NPPmsg->pos.x;
            playerInfo[NPPmsg->msg.head.id][1] = NPPmsg->pos.y;

            SendPosToGui(javaSocket);


            if (NPPmsg->msg.head.id == thisClientId) {
                // Update position
                x = NPPmsg->pos.x;
                y = NPPmsg->pos.y;
            }
            break;

        default:
            cout << "No change type msg!" << endl;
            break;
        }

        bufOffset += msg->head.length;
        if (bufOffset >= msgSize) {
            break;
        }
    }
}

/**
Recieves and decrypts the incomming messages from the GUI
@param the socket that connects to the server
@param the socket that connects to the java GUI
@param this clients ID
*/
void DoGUIwork(SOCKET javaSocket, SOCKET gameSocket, int thisClientId) {
    unsigned char buf[65536];
    sockaddr* address = 0;
    int* fromlen = 0;

    int msgSize = recvfrom(javaSocket, (char*)buf, sizeof(buf), 0, address, fromlen);

    if (thisClientId == buf[0]) {
        switch (buf[1])
        {
        case 0:
            // move up
            MoveFunc(x, y - 1, thisClientId, gameSocket);
            break;
        case 1:
            // move left
            MoveFunc(x - 1, y, thisClientId, gameSocket);
            break;
        case 2:
            // move down
            MoveFunc(x, y + 1, thisClientId, gameSocket);
            break;
        case 3:
            // move right
            MoveFunc(x + 1, y, thisClientId, gameSocket);
            break;
        default:
            break;
        }
    }
}

/**
Main function. sets up sockets and addresses to be used throughout the entire program for both sending and recieving
*/
void main() {
    /// Startup Winsock
    WSADATA data;
    WORD version MAKEWORD(2, 2);
    int wsOK = WSAStartup(version, &data);
    if (wsOK != 0) {
        cout << "Can't start Winsock! " << wsOK;
        return;
    }

    /// Create a hint structure for the server
    sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    //server.sin_port = htons(49152);
    server.sin_port = htons(9002);
    inet_pton(AF_INET, "192.168.1.65", &server.sin_addr);

    /// Socket creation
    SOCKET gameSocket = socket(AF_INET, SOCK_STREAM, 0);

    /// Connect to server and creates join message
    connect(gameSocket, (const sockaddr*)&server, sizeof(server));
    JoinMsg joining = {
        {0,0,0,Join},
        StaticObject,
        Cone,
        "bill"
    }; 
    joining.head.length = sizeof(joining);

    /// Sends joining package
    int sendOK = send(gameSocket, (const char*)&joining, joining.head.length, 0);

    // some variables
    char buf[65536];
    recv(gameSocket, buf, sizeof(buf), 0);
    ChangeMsg* msg = (ChangeMsg*)buf;
    int thisClientId = msg->head.id;
    std::cout << "My ID: " << thisClientId << std::endl;


//----------------------------------For JAVA------------------------------------------

    // Bind the ip address and port to a socket

    SOCKET javaSocket = socket(AF_INET6, SOCK_DGRAM, 0);
    sockaddr_in6 hint;
    memset(&hint, 0, sizeof(hint));
    hint.sin6_family = AF_INET6;
    hint.sin6_port = htons(54001);
    inet_pton(AF_INET6, "::1", &hint.sin6_addr);

    if (SOCKET_ERROR == bind(javaSocket, (const sockaddr*)&hint, sizeof(hint)))
        std::cout << GetLastError() << std::endl;

    // Create the master file descriptor set and zero it
    fd_set master;
    FD_ZERO(&master);

    FD_SET(gameSocket, &master);
    FD_SET(javaSocket, &master);

    // Movement inputs
    std::thread inputs([&thisClientId, &gameSocket, &javaSocket]() {
        while (true) {
            std::string command;
            cin >> command;
            if (command == "moveu") MoveFunc(x, y - 1, thisClientId, gameSocket);
            if (command == "moved") MoveFunc(x, y + 1, thisClientId, gameSocket);
            if (command == "movel") MoveFunc(x - 1, y, thisClientId, gameSocket);
            if (command == "mover") MoveFunc(x + 1, y, thisClientId, gameSocket);
            if (command == "q") { SendLeaveToServer(javaSocket, gameSocket, thisClientId); return; }
        }
    });
    //Send package to everyone
    while (true) {
        fd_set copy = master;
        
        if (select(0, &copy, nullptr, nullptr, nullptr) == -1) {
            std::cout << GetLastError() << endl;
            return;
        }

        if (FD_ISSET(gameSocket, &copy)) {
            DoWork(gameSocket, javaSocket, thisClientId);
        }
        
        if (FD_ISSET(javaSocket, &copy))
        {
            DoGUIwork(javaSocket, gameSocket, thisClientId);
        }
    }

    /// Create specified package
    
    closesocket(gameSocket);
    closesocket(javaSocket);
    WSACleanup();
}