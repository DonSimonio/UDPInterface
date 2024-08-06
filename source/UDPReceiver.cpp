#include "../include/UDPInterface.hpp"
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib

class UDPReceiverImp : public UDPReceiver {
public:
    UDPReceiverImp() : sockfd(INVALID_SOCKET) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    virtual ~UDPReceiverImp() {
        if (sockfd != INVALID_SOCKET) {
            closesocket(sockfd);
        }
        WSACleanup();
    }

    ERR_T connect(int port) override {
        sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sockfd == INVALID_SOCKET) {
            return ERR_T::SOCKET_CREATION_FAILED;
        }

        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
            return ERR_T::SET_FLAGS_FAILED;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(sockfd);
            sockfd = INVALID_SOCKET;
            return ERR_T::BIND_FAILED;
        }

        return ERR_T::OK;
    }

    ERR_T recv(uint8_t* buffer, size_t buffer_size, int& received_bytes) override {
        if (sockfd == INVALID_SOCKET) {
            return ERR_T::SOCKET_NOT_INITIALIZED;
        }

        received_bytes = ::recv(sockfd, reinterpret_cast<char*>(buffer), static_cast<int>(buffer_size), 0);
        if (received_bytes == SOCKET_ERROR) {
            return ERR_T::RECV_FAILED;
        }

        // Null-terminate the received data
        buffer[received_bytes] = '\0';

        return ERR_T::OK;
    }

    ERR_T setFlags(UDP_RECEIVER_FLAG flag, bool enable) override {
        int optval = enable ? 1 : 0;
        switch (flag) {
            case UDP_RECEIVER_FLAG::RECV_NONBLOCKING:
                if (ioctlsocket(sockfd, FIONBIO, (u_long*)&optval) == SOCKET_ERROR) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            case UDP_RECEIVER_FLAG::BROADCAST:
                if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char*)&optval, sizeof(optval)) == SOCKET_ERROR) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            default:
                return ERR_T::UNSUPPORTED_FLAG;
        }
        return ERR_T::OK;
    }

    ERR_T isDataAvailable() override {
        if (sockfd == INVALID_SOCKET) {
            return ERR_T::SOCKET_NOT_INITIALIZED;
        }

        WSAPOLLFD fds;
        fds.fd = sockfd;
        fds.events = POLLIN;

        int ret = WSAPoll(&fds, 1, 0);  // 0 timeout means non-blocking check
        if (ret > 0) {
            if (fds.revents & POLLIN) {
                return ERR_T::OK;
            }
        } else if (ret == 0) {
            return ERR_T::DATA_NOT_AVAILABLE;
        } else {
            return ERR_T::POLL_ERROR;
        }
        return ERR_T::UNDEFINED;
    }

private:
    SOCKET sockfd;
    int recvFlags = 0;
};


#elif defined(__unix__) || defined(ESP_PLATFORM)
#include <unistd.h>     // For close
#include <fcntl.h>      // For fcntl
#include <sys/socket.h> // For socket, setsockopt, recv
#include <arpa/inet.h>  // For sockaddr_in, htons, inet_pton

class UDPReceiverImp : public UDPReceiver {
public:
    UDPReceiverImp() : sockfd(-1) {}

    virtual ~UDPReceiverImp() {
        if (sockfd != -1) {
            close(sockfd);
        }
    }

    ERR_T connect(int port) override {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            return ERR_T::SOCKET_CREATION_FAILED;
        }

        int optval = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
            close(sockfd);
            sockfd = -1;
            return ERR_T::SET_FLAGS_FAILED;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
            close(sockfd);
            sockfd = -1;
            return ERR_T::BIND_FAILED;
        }

        return ERR_T::OK;
    }

    ERR_T recv(uint8_t* buffer, size_t buffer_size, int& received_bytes) override {
        if (sockfd == -1) {
            return ERR_T::SOCKET_NOT_INITIALIZED;
        }

        received_bytes = ::recv(sockfd, buffer, buffer_size, 0);
        if (received_bytes == -1) {
            return ERR_T::RECV_FAILED;
        }

        // Null-terminate the received data
        buffer[received_bytes] = '\0';

        return ERR_T::OK;
    }

    ERR_T setFlags(UDP_RECEIVER_FLAG flag, bool enable) override {
        int optval = enable ? 1 : 0;
        switch (flag) {
            case UDP_RECEIVER_FLAG::RECV_NONBLOCKING:
                if (fcntl(sockfd, F_SETFL, enable ? O_NONBLOCK : 0) == -1) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            case UDP_RECEIVER_FLAG::BROADCAST:
                if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
                    return ERR_T::SET_FLAGS_FAILED;
                }
                break;
            default:
                return ERR_T::UNSUPPORTED_FLAG;
        }
        return ERR_T::OK;
    }

    ERR_T isDataAvailable() override {
        if (sockfd == -1) {
            return ERR_T::SOCKET_NOT_INITIALIZED;
        }

        struct pollfd fds;
        fds.fd = sockfd;
        fds.events = POLLIN;

        int ret = poll(&fds, 1, 0);  // 0 timeout means non-blocking check
        if (ret > 0) {
            if (fds.revents & POLLIN) {
                return ERR_T::OK;
            }
        } else if (ret == 0) {
            return ERR_T::DATA_NOT_AVAILABLE;
        } else {
            return ERR_T::POLL_ERROR;
        }
        return ERR_T::UNDEFINED;
    }

private:
    int sockfd;
};

#else
#error "Unsupported platform"
#endif



std::shared_ptr<UDPReceiver> UDPInterface::createUDPReceiver() {
    return std::make_shared<UDPReceiverImp>();
}