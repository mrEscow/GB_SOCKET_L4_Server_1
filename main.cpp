//  Добавьте в какой-либо из вариантов сервера возможность отдавать части файла
//  по смещению и размеру

//#pragma comment(lib,"ws2_32.lib")

// IN PRO FILE: LIBS += D:\Qt\QT6\Tools\mingw1120_64\x86_64-w64-mingw32\lib\libws2_32.a  !!!

#include <QApplication>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <QDebug>
#include <thread>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <future>
//#include <iostream>
//#include <list>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <cerrno>

//const auto clients_count = 10;
const auto buffer_size = 8;
using namespace std::literals;
namespace fs = std::filesystem;

int test_count{0};

//------------------------------------------------------------------------------------
class Transceiver{
public:

    Transceiver(const SOCKET &client_sock) :
    client_sock_(client_sock) {}
    Transceiver() = delete;


public:
    bool send_buffer(const std::vector<char> &buffer)
    {
        size_t transmit_bytes_count = 0;
        const auto size = buffer.size();
        while (transmit_bytes_count != size)
        {
            auto result = send(client_sock_, &(buffer.data()[0]) +
            transmit_bytes_count,
            size - transmit_bytes_count, 0);
            if (-1 == result)
            {
                if (need_to_repeat()) continue;
                return false;
            }
            transmit_bytes_count += result;
        }
        return true;
    }

    bool send_file(fs::path const& file_path)
    {
        std::vector<char> buffer(buffer_size);
        std::ifstream file_stream(file_path);
        if (!file_stream) return false;
        qDebug() << "Sending file " << file_path.c_str() << "...\n";
        while (file_stream)
        {
            file_stream.read(&buffer[0], buffer.size());
            if (!send_buffer(buffer)) return false;
        }
        return true;
    }

    std::string get_request()
    {
        std::array<char, MAX_PATH + 1> buffer;
        size_t recv_bytes = 0;
        const auto size = buffer.size() - 1;
        qDebug() << "Reading user request...\n";
        while (true)
        {
            auto result = recv(client_sock_,
            &buffer[recv_bytes], size - recv_bytes, 0);
            if (!result) break;
            if (-1 == result)
            {
                if (need_to_repeat()) continue;
                throw std::logic_error("Socket reading error");
            }
            auto fragment_begin = buffer.begin() + recv_bytes;
            auto ret_iter = std::find_if(fragment_begin,
                fragment_begin + result,
                [](char sym)
                {
                return '\n' == sym ||
                '\r' == sym;
                });
            if (ret_iter != buffer.end())
            {
                *ret_iter = '\0';
                recv_bytes += std::distance(fragment_begin, ret_iter);
                break;
            }
            recv_bytes += result;
            if (size == recv_bytes) break;
        }
        buffer[recv_bytes] = '\0';
        auto result = std::string(buffer.begin(),
        buffer.begin() + recv_bytes);
        qDebug() << "Request = \"" << result.c_str() << "\"";
        return result;
    }

private:
    static bool need_to_repeat()
    {
        switch (errno)
        {
        case EINTR:
        case EAGAIN:
        // case EWOULDBLOCK: // EWOULDBLOCK == EINTR.
        return true;
        }
        return false;
    };
private:
    SOCKET client_sock_;

};
//------------------------------------------------------------------------------------
class Client{
    QString m_name;
    SOCKET m_socket;
    sockaddr_in m_Info;
    Transceiver * m_transceiver;
    QVector<char> clientBuff;//(buffer_size);
    QVector<char> servBuff;
    bool is_delete{false};
public:
    Client(const SOCKET &socket, const sockaddr_in &Info)
    {
        m_socket = socket;
        m_Info = Info;

        m_transceiver = new Transceiver(socket);

        qDebug() << "Connection to a client established successfully";
        char clientIP[22];
        inet_ntop(AF_INET, &m_Info.sin_addr, clientIP, INET_ADDRSTRLEN);	// Convert connected client's IP to standard string format
        qDebug() << "Client connected with IP address " << clientIP;
        qDebug() << "Client discriptor " << m_socket;

        std::thread thr_Recv([&](){
            ClientRecv();
        });

        thr_Recv.detach();
        qDebug() << "CLIENT: thr_Recv.detach() ";

    };

    Client() = delete;

    void ClientRecv(){

        //Sleep(1000);
        int recv_size{-2};
        QString full_msg;
        servBuff.resize(buffer_size);

        while(!is_delete){

             recv_size = recv(m_socket, servBuff.data(), servBuff.size(), 0);

            if(recv_size == -1){
                qDebug() << "ERROR_RECV: closesocket(Client)";
                closesocket(m_socket);
                is_delete = true;
                //continue;
            }

            if(recv_size == 0){
                if(!full_msg.isEmpty()){
                    qDebug() << "SIZE_RECV" << recv_size << "full_msg:" << full_msg;
                    full_msg.clear();
                }
                //continue;
            }

            if(recv_size == servBuff.size()){
                for(int i = 0; i < recv_size;i++){
                    full_msg += servBuff.data()[i];
                }
                qDebug() << "SIZE_RECV" << recv_size << "full_msg:" << full_msg;
            }

            if(recv_size < servBuff.size() && recv_size > 0){
                for(int i = 0; i < recv_size;i++){
                    full_msg += servBuff.data()[i];
                }
                qDebug() << "SIZE_RECV" << recv_size << "full_msg:" << full_msg;
                full_msg.clear();
            }
        }
    }

    bool SendMessage(const QString &msg){
        clientBuff.resize(msg.size());
        //int SizeMsg = msg.size();
        for(int i = 0; i < msg.size();i++){
            clientBuff[i] = msg.toStdString()[i];
        }
        qDebug() << test_count++ << "MSG:" << clientBuff.data() << "SIZE:" << clientBuff.size();

        if(send(m_socket,clientBuff.data(),clientBuff.size(),0) != SOCKET_ERROR){
            return true;
        }
        else{
            //closesocket(m_socket);
            qDebug() << "SEND CLIENT ERROR:" << WSAGetLastError();
            //is_delete = true;
            return false;
        }

    };

    bool Is_DELETE(){
        return is_delete;
    }

    ~Client(){
       closesocket(m_socket);
       delete m_transceiver;
    }
};
//------------------------------------------------------------------------------------
class Server{
    SOCKET m_server_socket;
    SOCKADDR_IN m_addr;
    int m_addrSize;
    QVector<Client> m_Clients;
    bool m_stop{false};
    std::thread thr;

public:
    Server()= delete;
    Server(const SOCKADDR_IN &addr)
    {
        m_server_socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if (m_server_socket == INVALID_SOCKET) {
            qDebug() << "Error initialization socket # " << WSAGetLastError();
            closesocket(m_server_socket);
            WSACleanup();
            exit(1);
        }
        else
            qDebug() << "Server socket initialization is OK";

        m_addr = addr;
        m_addrSize = sizeof(m_addr);

        int erStat = bind(m_server_socket,(SOCKADDR*)&m_addr,sizeof(m_addr));
        if ( erStat != 0 ) {
            qDebug() << "Error Socket binding to server info. Error # " << WSAGetLastError();
            closesocket(m_server_socket);
            WSACleanup();
            exit(2);
        }
        else
            qDebug() << "Binding socket to Server info is OK";

        qDebug() << "SOMAXCONN:" << SOMAXCONN;
        erStat = listen(m_server_socket, SOMAXCONN);
        if ( erStat != 0 ) {
            qDebug() << "Can't start to listen to. Error # " << WSAGetLastError();
            closesocket(m_server_socket);
            WSACleanup();
            exit(3);
        }
        else {
            qDebug() << "Listening...";
        }

        std::thread thr_Check_Clients([&](){
            while(!m_stop){
                for(QVector<Client>::Iterator it = m_Clients.begin(); it != m_Clients.end();)
                {
                    if(it->Is_DELETE())
                    {
                        it = m_Clients.erase(it);
                    }
                    else it++;
                }
            }
        });

        thr_Check_Clients.detach();
        qDebug() << "thr_Check_Clients.detach() ";

        std::thread thr_Accept_Clients([&](){
            while(!m_stop){
                sockaddr_in ClientInfo;
                ZeroMemory(&ClientInfo, sizeof(ClientInfo));	// Initializing clientInfo structure
                int clientInfo_size = sizeof(ClientInfo);

                SOCKET ClientConn = accept(m_server_socket, (sockaddr*)&ClientInfo, &clientInfo_size);

                if (ClientConn == INVALID_SOCKET) {
                    qDebug() << "Client detected, but can't connect to a client. Error # " << WSAGetLastError();
                    closesocket(m_server_socket);
                    closesocket(ClientConn);
                    WSACleanup();
                    exit(4);
                }
                else {
                    //qDebug() << "Connection to a client established successfully";
                    //char clientIP[22];
                    //inet_ntop(AF_INET, &ClientInfo.sin_addr, clientIP, INET_ADDRSTRLEN);	// Convert connected client's IP to standard string format
                    //qDebug() << "Client connected with IP address " << clientIP;
                    //qDebug() << "Client discriptor " << ClientConn;
                    m_Clients.push_back(Client(ClientConn, ClientInfo));
                }
            }
        });


        thr_Accept_Clients.detach();
        qDebug() << "thr_Accept_Clients.detach() ";
    }
    ~Server(){
        m_stop = true;
        closesocket(m_server_socket);
    }
    void Stop_server(){
        m_stop = true;
    }

    void Send_message(const QString msg){
        for(auto client: m_Clients)
            client.SendMessageW(msg);

    }
};
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // init WSA for windows
    WSAData wsaData; // информация об интерфейсах компутера
    WORD DllVersion = MAKEWORD(2,2); // (от второй до второй вернсии) uint 16 x << 8 | y = 514
    if(WSAStartup(DllVersion, &wsaData) !=0){
        qDebug() << "Error: WSAStartup";
        return a.exec();
    }

    // init IP, port and famaly
    SOCKADDR_IN addr;
    //addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addr.sin_addr.S_un.S_addr = INADDR_ANY;
    addr.sin_port = htons(1989);
    addr.sin_family = AF_INET;


    Server server(addr);

    int stop = 50;
    while(true){
        Sleep(2000);
        //server.Send_message("Test message!");
        stop--;
    }

    //server.Stop_server();

    // Cleanup winsock
    WSACleanup();
    qDebug()<<"exit program";
    return a.exec();
}
