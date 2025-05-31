#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <unistd.h>
#include <memory>
#include <cstring>

using namespace std;

class Player {
private:
    std::string name;
    int id;
    int client_fd;
    int main_fd;
    bool has_name;
    bool is_playing;
    int room;
    int index;
    int wins;

public:
    Player(int fd, int my_index);
    string get_name();
    bool is_get_name();
    bool is_client_playing();
    void set_name(std::string inp_name);
    void set_fd(int fd);
    void set_playing_mode(bool mode, int room_id);
    void send_me_message(const char* message);
    int get_room();
    int get_fd();
    int get_main_fd();
    int get_index();
    int get_wins();
    void add_wins();
};

#endif
