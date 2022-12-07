#define WIN32_LEAN_AND_MEAN
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<string>
#include <chrono>
#include <WS2tcpip.h>
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")

using namespace std;

void main() {
    /// Info for each pixel to be sent
    struct PixelInfo {
        BYTE x;
        BYTE y;
        BYTE color;
    };

    /// Holds all info to be sent via network
    struct SendInfo {
        short amountOfPixels;
        PixelInfo pixels[5120];
    }pack;

    /// Startup Winsock
    WSADATA data;
    WORD version MAKEWORD(2, 2);
    int wsOK = WSAStartup(version, &data);
    if (wsOK != 0) {
        cout << "Can't start Winsock! " << wsOK;
    }

    /// Create a hint structure for the server
    sockaddr_in6 server;
    memset(&server, 0, sizeof(server));
    server.sin6_family = AF_INET6;
    server.sin6_port = htons(54000);
    inet_pton(AF_INET6, "::1", &server.sin6_addr);

    /// Socket creation
    SOCKET out = socket(AF_INET6, SOCK_DGRAM, 0);

    /// Write out to that socket
    SendInfo info;
    info.amountOfPixels = 2400;
    int color = 0;

    while (color <= 8) {
        /// Wait 5 seconds
        if (color > 0) {
            Sleep(5000);
        }

        /// Set pixel colors and coords
        int index = 0;
        for (int i = 0; i < 200; i += 4)
        {
            for (int j = 0; j < 200; j += 4)
            {
                info.pixels[index] = PixelInfo{ (unsigned char)j,(unsigned char)i,(unsigned char)(color) };
                index++;
            }
        }

        /// Sends package
        int sendOK = sendto(out, (const char*)&info, sizeof(PixelInfo) * info.amountOfPixels + 2, 0, (sockaddr*)&server, sizeof(server));

        /// Check if package got sent Ok
        if (sendOK == SOCKET_ERROR) {
            cout << "That didn't work " << WSAGetLastError() << endl;
        }
        color++;
    }

    /// Close socket and cleanup WSA
    closesocket(out);
    WSACleanup();
}