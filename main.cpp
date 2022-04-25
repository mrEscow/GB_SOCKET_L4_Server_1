//  Добавьте в какой-либо из вариантов сервера возможность отдавать части файла
//  по смещению и размеру

//#pragma comment(lib,"ws2_32.lib")

// IN PRO FILE: LIBS += D:\Qt\QT6\Tools\mingw1120_64\x86_64-w64-mingw32\lib\libws2_32.a  !!!

#include <QApplication>
#include <winsock2.h>
#include <QDebug>
#include <thread>


//bool send_buffer(const std::vector<char> &buffer,int index)
//{
//    size_t transmit_bytes_count = 0;

//    const auto size = buffer.size();

//    while (transmit_bytes_count != size)
//    {
//        auto result = send(
//                    Connections[index],
//                    &(buffer.data()[0]) + transmit_bytes_count,
//                    size - transmit_bytes_count,
//                    NULL);

//            transmit_bytes_count += result;

//    }
//    return true;
//}


class Client{
    QString m_name;
    SOCKET m_socket;
public:
    Client(SOCKET socket): m_socket(socket){
        QString msg = "Hello from Server!";
        int SizeMsg = msg.size();

        send(m_socket,(char*)SizeMsg,sizeof(int),NULL);
        send(m_socket, msg.toStdString().c_str(),SizeMsg, NULL);
    };
    Client() = delete;

    void SendMessage(const QString msg){

        int SizeMsg = msg.size();

        send(m_socket,(char*)SizeMsg,sizeof(int),NULL);
        send(m_socket, msg.toStdString().c_str(),SizeMsg, NULL);
    };

    ~Client(){
       closesocket(m_socket);
    }
};

class Server{
    SOCKET m_socket;
    SOCKADDR_IN m_addr;
    int m_addrSize;
    QList<Client> m_Clients;
    bool m_stop{false};
public:
    Server()= delete;
    Server(SOCKET socket,SOCKADDR_IN addr)
    {
        m_socket = socket;
        m_addr = addr;
        m_addrSize = sizeof(m_addr);
        bind(m_socket,(SOCKADDR*)&m_addr,sizeof(m_addr));
        listen(m_socket,SOMAXCONN);

    }
    ~Server(){
        m_stop = true;
        closesocket(m_socket);
    }
    void Stop_server(){
        m_stop = true;
    }
    void Run_server(){
        m_stop = false;
        std::thread thr([&](){
            while(!m_stop){
                SOCKET newConnection = accept( m_socket ,(SOCKADDR*) &m_addr, &m_addrSize);
                m_Clients << newConnection;
                closesocket(newConnection);
            }
        });

        thr.join();
    }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // init WSA for windows
    WSAData wsaData;
    WORD DllVersion = MAKEWORD(2,2);
    if(WSAStartup(DllVersion, &wsaData) !=0){
        qDebug() << "Error: WSAStartup";
        return a.exec();
    }

    // init IP port and famaly
    SOCKADDR_IN addr;
    addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1989);
    addr.sin_family = AF_INET;

    // init discriptor
    SOCKET sListen = socket(AF_INET,SOCK_STREAM,NULL);

    Server server(sListen, addr);
    server.Run_server();
    // Cleanup winsock
    WSACleanup();
    qDebug()<<"exit";
    return a.exec();
}
