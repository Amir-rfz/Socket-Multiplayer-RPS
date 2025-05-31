#ifndef ROOM_H
#define ROOM_H

#include <vector>
#include <array>
#include <string>
#include <poll.h>
#include <memory>
#include <netinet/in.h>
#include <vector>
#include "player.h"

using namespace std;

class Room {
private:
    int id;
    int room_fd;
    int port;
    std::vector<std::shared_ptr<Player>> clients;
    std::vector<struct pollfd> pfds;
    std::array<int, 2> answers;
    bool is_start;
    int bc_sock;
    struct sockaddr_in bc_address;

public:
    Room(int fd, int inp_port, int id, int bc_sock, struct sockaddr_in bc_address);
    std::string get_room_list(std::vector<std::shared_ptr<Room>>& all_rooms, int self_id);
    int get_fd();
    int get_port();
    int get_number_of_clients();
    void add_client(std::shared_ptr<Player> client);
    void send_message_to(const char* message, int index);
    void send_message_to_all(const char* message);
    void set_ans(int fd, int ans);
    bool is_game_end();
    void free_room();
    int get_last_client_index();
    int get_winner_result();
    void handle_game(int self_id);
};

#endif
