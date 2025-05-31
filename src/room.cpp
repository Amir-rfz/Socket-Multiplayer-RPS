#include "player.h"
#include "room.h"
#include <cstring>
#include <iostream>
#include <unistd.h>
const char* ROOM_WINNER = "The game is done in room ";

Room::Room(int fd, int inp_port, int id, int bc_sock, struct sockaddr_in bc_address)
    : room_fd(fd), bc_sock(bc_sock), bc_address(bc_address), port(inp_port), id(id), 
      answers{-1, -1}, is_start(false) {
    pfds.push_back(pollfd{room_fd, POLLIN, 0});
}

string Room::get_room_list(vector<shared_ptr<Room>>& all_rooms, int self_id) {
    string room_list = "All Rooms:\n";
    for (int j = 0; j < all_rooms.size(); ++j) {
        if (j == self_id) {
            room_list += "Room " + to_string(j + 1) + " (2 place remind)\n";
            continue;
        }
        int room_size = all_rooms[j]->get_number_of_clients();
        string available_msg = " (" + to_string(2 - room_size) + " place remind)\n";
        string full_msg = " (Full)\n";
        room_list += "Room " + to_string(j + 1) + (room_size < 2 ? available_msg : full_msg);
    }
    return room_list;
}

int Room::get_fd() {
    return room_fd;
}

int Room::get_port() {
    return port;
}

int Room::get_number_of_clients() {
    return clients.size();
}

void Room::add_client(shared_ptr<Player> client) {
    clients.push_back(client);
}

void Room::send_message_to(const char* message, int index) {
    clients[index]->send_me_message(message);
}

void Room::send_message_to_all(const char* message) {
    for (int i = 0; i < clients.size(); i++) {
        send_message_to(message, i);
    }
}

void Room::set_ans(int fd, int ans) {
    if (fd == clients[0]->get_fd() && answers[0] == -1)
        answers[0] = ans;
    if (fd == clients[1]->get_fd() && answers[1] == -1)
        answers[1] = ans;
}

bool Room::is_game_end() {
    return (answers[0] != -1 && answers[1] != -1);
}

void Room::free_room() {
    for (int i = 0; i < clients.size(); i++) {
        clients[i]->set_playing_mode(false, -1);
        int main_fd = clients[i]->get_main_fd();
        answers[i] = -1;
    }
    clients.clear();
    is_start = 0;
}

int Room::get_last_client_index() {
    if (clients.size() == 1) {
        return clients[0]->get_index();
    }
    if (clients.size() == 2) {
        return clients[1]->get_index();
    }
    return -1;
}

int Room::get_winner_result() {
    if (answers[0] == -1 && answers[1] == -1)
        return -1;
    else if (answers[0] != -1 && answers[1] == -1)
        return 0;
    else if (answers[0] == -1 && answers[1] != -1)
        return 1;

    if (answers[0] == answers[1])
        return -1;
    else if (answers[0] == 1 && answers[1] == 2)
        return 1;
    else if (answers[0] == 1 && answers[1] == 3)
        return 0;
    else if (answers[0] == 2 && answers[1] == 1)
        return 0;
    else if (answers[0] == 2 && answers[1] == 3)
        return 1;
    else if (answers[0] == 3 && answers[1] == 1)
        return 1;
    else if (answers[0] == 3 && answers[1] == 2)
        return 0;

    return -2;
}

void Room::handle_game(int self_id) {
    int winner_result = get_winner_result();

    if (winner_result == -1) {
        send_message_to_all("Time's up! The game is over\n");
        string res = ROOM_WINNER + to_string(this->id) + " : " + " The game resulted in a tie\n";
        int result = sendto(bc_sock, res.c_str(), res.size(), 0, (struct sockaddr*)&bc_address, sizeof(bc_address));
        if (result == -1) {
            perror("Broadcast failed");
        }
    }
    else if (winner_result == 0) {
        string win_res = ROOM_WINNER + to_string(this->id) + " : " + clients[0]->get_name() + " win\n";
        int result = sendto(bc_sock, win_res.c_str(), win_res.size(), 0, (struct sockaddr*)&bc_address, sizeof(bc_address));
        if (result == -1) {
            perror("Broadcast failed");
        }
        clients[0]->add_wins();
    }
    else if (winner_result == 1) {
        string win_res = ROOM_WINNER + to_string(this->id) + " : " + clients[1]->get_name() + " win\n";
        int result = sendto(bc_sock, win_res.c_str(), win_res.size(), 0, (struct sockaddr*)&bc_address, sizeof(bc_address));
        if (result == -1) {
            perror("Broadcast failed");
        }
        clients[1]->add_wins();
    }
}
