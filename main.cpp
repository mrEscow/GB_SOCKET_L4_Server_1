//  Добавьте в какой-либо из вариантов сервера возможность отдавать части файла
//  по смещению и размеру

//#pragma comment(lib,"ws2_32.lib")

// IN PRO FILE: LIBS += D:\Qt\QT6\Tools\mingw1120_64\x86_64-w64-mingw32\lib\libws2_32.a  !!!

#include <QApplication>

#include <winsock2.h>

#include <QDebug>

SOCKET Connections[100];
int ClientCount = 0;

void ClientHandle(int index){
    char msg[256];

    while(true){
        recv(Connections[index],msg,sizeof(msg),NULL);

        for(int i=0;i<ClientCount;i++){
            if(i==index){
                continue;
            }
            send(Connections[i],msg,sizeof(msg),NULL);
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    WSAData wsaData;
    WORD DllVersion = MAKEWORD(2,2);
    if(WSAStartup(DllVersion, &wsaData) !=0){
        qDebug() << "Error: WSAStartup";
        return a.exec();
    }

    SOCKADDR_IN addr;
    addr.sin_addr.S_un.S_addr =inet_addr("127.0.0.1");
    addr.sin_port = htons(1989);
    addr.sin_family = AF_INET;

    SOCKET sListen = socket(AF_INET,SOCK_STREAM,NULL);

    bind(sListen,(SOCKADDR*)&addr,sizeof(addr));

    listen(sListen,SOMAXCONN);

    SOCKET newConnection;
    int addrSize = sizeof(addr);

    for(int i=0;i<100;i++){
        newConnection = accept(sListen,(SOCKADDR*)&addr,&addrSize);

        if(newConnection == 0){
            qDebug() << "Error: Client Connection";
        }
        else{
            qDebug() << "Client Connection";
            char msg[256] = "Hello from Server";
            send(newConnection, msg,sizeof(msg), NULL);

            Connections[i] = newConnection;
            ClientCount++;

            CreateThread(NULL,NULL,(LPTHREAD_START_ROUTINE)ClientHandle,(LPVOID)(i),NULL,NULL);
        }
    }



    return a.exec();
}
