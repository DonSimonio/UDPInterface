#include "../include/UDPInterface.hpp"
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>   // For Windows Sockets
#include <ws2tcpip.h>   // For inet_pton
#pragma comment(lib, "ws2_32.lib")

class UDPSenderImp : public UDPSender {
public:
    UDPSenderImp() : sockfd(INVALID_SOCKET) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    virtual ~UDPSenderImp() {
        if (sockfd != INVALID_SOCKET) {
            closesocket(sockfd);
        }
        WSACleanup();
    }
    
    ERR_T connect(std::string ip, int port) override {
        if (sockfd != INVALID_SOCKET) {
            return ERR_T::SOCKET_ALREADY_CONNECTED;
        }

        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockfd == INVALID_SOCKET) {
            WSACleanup();
            return ERR_T::SOCKET_CREATION_FAILED;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
            closesocket(sockfd);
            WSACleanup();
            return ERR_T::INVALID_IP;
        }
        m_ipAddr = ip;
        m_port = port;

        // std::cout << "Connected to " << ip << ":" << port << std::endl;
        return ERR_T::OK;
    }

    ERR_T send(const void* data, size_t length) override {
        if (sockfd == INVALID_SOCKET) {
            return ERR_T::SOCKET_NOT_INITIALIZED;
        }

        int sent_len = sendto(sockfd, reinterpret_cast<const char*>(data), static_cast<int>(length), 0,
                              reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if (sent_len == SOCKET_ERROR) {
            return ERR_T::SEND_FAILED;
        } else {
            return ERR_T::OK;
        }
    }

    ERR_T setFlags(UDP_SENDER_FLAG flag, bool enable) override {
        int optval = enable ? 1 : 0;
        switch (flag) {
            case UDP_SENDER_FLAG::SEND_NONBLOCKING:
                if (ioctlsocket(sockfd, FIONBIO, (u_long*)&optval) == SOCKET_ERROR) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            case UDP_SENDER_FLAG::BROADCAST:
                // check ip is 255.255.255.255, if not set, passtrough, if not 255.255.255.255 return ERR_T::SET_FLAGS_FAILED
                if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            default:
                return ERR_T::UNSUPPORTED_FLAG;
        }
        return ERR_T::OK;
    }

private:
    SOCKET sockfd;
    sockaddr_in server_addr;
    std::string m_ipAddr = "";
    int m_port = -1;
};


#elif defined(__unix__) || defined(ESP_PLATFORM)
#include <sys/socket.h> // For socket, sendto, setsockopt, etc.
#include <arpa/inet.h>  // For sockaddr_in, inet_pton
#include <unistd.h>     // For close
#include <fcntl.h>      // For fcntl
#include <cstring>      // For memset

class UDPSenderImp : public UDPSender {
public:
    UDPSenderImp() : sockfd(-1) {}

    virtual ~UDPSenderImp() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }
    
    ERR_T connect(std::string ip, int port) override {
        if (sockfd != -1) {
            return ERR_T::SOCKET_ALREADY_CONNECTED;
        }

        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            return ERR_T::SOCKET_CREATION_FAILED;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
            close(sockfd);
            return ERR_T::INVALID_IP;
        }
        m_ipAddr = ip;
        m_port = port;

        return ERR_T::OK;
    }

    ERR_T send(const void* data, size_t length) override {
        if (sockfd == -1) {
            return ERR_T::SOCKET_NOT_INITIALIZED;
        }

        ssize_t sent_len = sendto(sockfd, data, length, 0,
                                  reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
        if (sent_len == -1) {
            return ERR_T::SEND_FAILED;
        } else {
            return ERR_T::OK;
        }
    }

    ERR_T setFlags(UDP_SENDER_FLAG flag, bool enable) override {
        int optval = enable ? 1 : 0;
        switch (flag) {
            case UDP_SENDER_FLAG::SEND_NONBLOCKING:
                if (fcntl(sockfd, F_SETFL, enable ? O_NONBLOCK : 0) == -1) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            case UDP_SENDER_FLAG::BROADCAST:
                if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            default:
                return ERR_T::UNSUPPORTED_FLAG;
        }
        return ERR_T::OK;
    }

private:
    int sockfd;
    sockaddr_in server_addr;
    std::string m_ipAddr = "";
    int m_port = -1;
};

#else
#error "Unsupported platform"
#endif




std::shared_ptr<UDPSender> UDPInterface::createUDPSender() {
    return std::make_shared<UDPSenderImp>();
}