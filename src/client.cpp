#include "client.h"

int initialize_tcp_socket(char* ipaddr, int port) {
    struct sockaddr_in server_addr;
    int server_fd, opt = 1;

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ipaddr, &(server_addr.sin_addr));
    server_addr.sin_port = htons(port);
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));

    server_fd = socket(PF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    if (connect(server_fd, (sockaddr*)(&server_addr), sizeof(server_addr)) == -1) {
        perror("FAILED: Connect");
    }
    return server_fd;
}

int initialize_broadcast_socket(int port) {
    struct sockaddr_in bc_address;
    int bc_sock, broadcast = 1, opt = 1;

    bc_sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(bc_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(bc_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(port);
    bc_address.sin_addr.s_addr = inet_addr("127.255.255.255");
    bind(bc_sock, (struct sockaddr*)&bc_address, sizeof(bc_address));

    return bc_sock;
}

// Handle incoming messages for when we want to connect client to a room
void handle_server_message(int fd, const char* ipaddr, vector<pollfd>& pfds, int& server_fd, int main_server) {
    char buffer[BUFFER_SIZE] = {0};
    recv(fd, buffer, BUFFER_SIZE, 0);
    std::string prefix = "Room:";
    std::string main_server_str = MAIN_SERVER;
    std::string buffer_str = buffer;

    if (buffer_str.compare(0, prefix.length(), prefix) == 0) {
        int room_port = stoi(buffer_str.substr(prefix.length()));
        int room_fd = initialize_tcp_socket(const_cast<char*>(ipaddr), room_port);
        pfds[0] = pollfd{room_fd, POLLIN, 0};
        server_fd = room_fd;
    } else if (buffer_str.compare(0, main_server_str.length(), main_server_str) == 0) {
        pfds[0] = pollfd{main_server, POLLIN, 0};
    } else {
        write(STDOUT_FILENO, buffer, strlen(buffer));
    }
}

void handle_user_input(int fd, vector<pollfd>& pfds) {
    char buffer[BUFFER_SIZE] = {0};
    read(STDIN_FILENO, buffer, BUFFER_SIZE);
    send(fd, buffer, strlen(buffer), 0);
}

void handle_broadcast_message(int bc_sock) {
    char bc_buffer[1024] = {0};
    int bytes_received = recv(bc_sock, bc_buffer, sizeof(bc_buffer), 0);
    if (bytes_received > 0) {
        write(STDOUT_FILENO, bc_buffer, bytes_received);
    } else if (bytes_received == -1) {
        perror("Broadcast receive failed");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        perror("Invalid Arguments");
        return EXIT_FAILURE;
    }

    char* ipaddr = argv[1];
    int port = stoi(argv[2]);

    int server_fd = initialize_tcp_socket(ipaddr, port);
    int main_server = server_fd;
    int bc_sock = initialize_broadcast_socket(port);

    vector<pollfd> pfds = {
        {server_fd, POLLIN, 0},
        {STDIN_FILENO, POLLIN, 0},
        {bc_sock, POLLIN, 0}
    };

    while (true) {
        if (poll(pfds.data(), pfds.size(), -1) == -1) {
            perror("FAILED: Poll");
            return EXIT_FAILURE;
        }

        for (auto& pfd : pfds) {
            if (pfd.revents & POLLIN) {
                if (pfd.fd == server_fd) {
                    handle_server_message(pfd.fd, ipaddr, pfds, server_fd, main_server);
                } else if (pfd.fd == STDIN_FILENO) {
                    handle_user_input(pfds[0].fd, pfds);
                } else if (pfd.fd == bc_sock) {
                    handle_broadcast_message(bc_sock);
                }
            }
        }
    }
    return EXIT_SUCCESS;
}
