enum ERR_T {
    UNDEFINED = -1,
    OK = 0,

    /* CONNECT */
    SOCKET_ALREADY_CONNECTED,
    SOCKET_CREATION_FAILED,
    INVALID_IP,
    BIND_FAILED,

    /* SEND */
    SOCKET_NOT_INITIALIZED,
    SEND_FAILED,

    /* RECV */
    RECV_FAILED,

    /* SET FLAGS*/
    SET_FLAGS_FAILED,
    UNSUPPORTED_FLAG,

    /* IS DATA AVAILABLE */
    DATA_NOT_AVAILABLE,
    POLL_ERROR
};

enum class UDP_SENDER_FLAG {
    SEND_NONBLOCKING,
    BROADCAST
};

enum class UDP_RECEIVER_FLAG {
    RECV_NONBLOCKING,
    BROADCAST
};