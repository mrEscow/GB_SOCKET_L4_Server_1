//  Добавьте в какой-либо из вариантов сервера возможность отдавать части файла
//  по смещению и размеру

//#pragma comment(lib,"ws2_32.lib")

// IN PRO FILE: LIBS += D:\Qt\QT6\Tools\mingw1120_64\x86_64-w64-mingw32\lib\libws2_32.a  !!!

#include <QApplication>
#include <winsock2.h>
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
const auto buffer_size = 4096;
using namespace std::literals;
namespace fs = std::filesystem;

//------------------------------------------------------------------------------------
class Transceiver{
public:

    Transceiver(SOCKET &&client_sock) :
    client_sock_(std::move(client_sock)) {}
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
    Transceiver m_transceiver;
public:
    Client(SOCKET socket)
        : m_socket(socket),
          m_transceiver(std::move(socket))
    {
        QString msg = "Hello from Server!";
        int SizeMsg = msg.size();

        send(m_socket,(char*)SizeMsg,sizeof(int),NULL);
        send(m_socket, msg.toStdString().c_str(),SizeMsg, NULL);
    };

    Client() = delete;

    void SendMessage(const QString msg){

        int SizeMsg = msg.size();
        qDebug() << "MSG:" << msg << "SIZE:" << SizeMsg;
        send(m_socket,(char*)SizeMsg,sizeof(int),NULL);
        send(m_socket, msg.toStdString().c_str(), SizeMsg, NULL);
    };

    ~Client(){
       closesocket(m_socket);
    }
};
//------------------------------------------------------------------------------------
class Server{
    SOCKET m_socket;
    SOCKADDR_IN m_addr;
    int m_addrSize;
    QList<Client*> m_Clients;
    bool m_stop{false};
    std::thread thr;
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
                m_Clients.push_back(new Client(newConnection));
                //closesocket(newConnection);
            }
        });

        thr.detach();
    }
    void Send_message(const QString msg){
        for(auto client: m_Clients)
            client->SendMessageW(msg);
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
    addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1989);
    addr.sin_family = AF_INET;

    // init discriptor
    SOCKET sListen = socket(AF_INET,SOCK_STREAM,NULL);

    Server server(sListen, addr);
    server.Run_server();

    while(true){
        Sleep(1000);
        server.Send_message("Test message!");
    }


    // Cleanup winsock
    WSACleanup();
    qDebug()<<"exit";
    return a.exec();
}
