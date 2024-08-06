#pragma once

#include <memory>
#include <string>
#include "UDPEnums.hpp"
class UDPSender;
class UDPReceiver;

class UDPInterface {
public:
    virtual ~UDPInterface() = default;

    static std::shared_ptr<UDPSender> createUDPSender();
    static std::shared_ptr<UDPReceiver> createUDPReceiver();
};

class UDPSender : public UDPInterface {
public:
    virtual ERR_T connect(std::string ip, int port) = 0;
    virtual ERR_T send(const void* data, size_t length) = 0;
    virtual ERR_T setFlags(UDP_SENDER_FLAG flag, bool enable = true) = 0;

    virtual ~UDPSender() = default;

    UDPSender(const UDPSender&) = delete;
    UDPSender& operator=(const UDPSender&) = delete;

    UDPSender(UDPSender&&) noexcept = default;
    UDPSender& operator=(UDPSender&&) noexcept = default;
    
protected:
    UDPSender() = default;
};

class UDPReceiver : public UDPInterface {
public:
    virtual ERR_T connect(int port) = 0;
    virtual ERR_T recv(uint8_t* buffer, size_t buffer_size, int& received_bytes) = 0;
    virtual ERR_T setFlags(UDP_RECEIVER_FLAG flag, bool enable = true) = 0;
    virtual ERR_T isDataAvailable() = 0;

    virtual ~UDPReceiver() = default;

    UDPReceiver(const UDPReceiver&) = delete;
    UDPReceiver& operator=(const UDPReceiver&) = delete;

    UDPReceiver(UDPReceiver&&) noexcept = default;
    UDPReceiver& operator=(UDPReceiver&&) noexcept = default;

protected:
    UDPReceiver() = default;
};