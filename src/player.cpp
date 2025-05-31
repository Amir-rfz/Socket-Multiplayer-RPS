#include "player.h"

Player::Player(int fd, int my_index) : client_fd(fd), main_fd(fd), index(my_index), wins(0),
                                        has_name(false), is_playing(false) {}

bool Player::is_get_name() {
    return has_name;
}

bool Player::is_client_playing() {
    return is_playing;
}

void Player::set_name(string inp_name) {
    name = inp_name.erase(inp_name.size() - 1); 
    has_name = true;
}

void Player::set_fd(int fd) {
    client_fd = fd;
}

void Player::set_playing_mode(bool mode, int room_id) {
    is_playing = mode;
    room = room_id;
}

void Player::send_me_message(const char* message) {
    write(client_fd, message, strlen(message));
}

int Player::get_room() {
    return room;
}

int Player::get_fd() {
    return client_fd;
}

int Player::get_main_fd() {
    return main_fd;
}

int Player::get_index() {
    return index;
}

int Player::get_wins() {
    return wins;
}

string Player::get_name() {
    return name;
}

void Player::add_wins() {
    wins += 1;
}
