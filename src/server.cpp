#include "server.h"
#include "player.h"
#include "room.h"

map<int, timer_t> active_timers;  
map<int, int> room_timer_map;      
int current_timer_id = 0;
vector<shared_ptr<Room>> rooms;

void send_message(int fd, const char* message) {
    write(fd, message, strlen(message));
}

int setup_server(char* ipaddr, int port) {
    struct sockaddr_in server_addr;
    int server_fd, opt = 1;
    server_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, ipaddr, &(server_addr.sin_addr)) == -1)
        perror("FAILED: Input ipv4 address invalid");
    if((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        perror("FAILED: Socket was not created");
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable failed");
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        perror("FAILED: Making socket reusable port failed");
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    server_addr.sin_port = htons(port);
    if(bind(server_fd, (const struct sockaddr*)(&server_addr), sizeof(server_addr)) == -1)
        perror("FAILED: Bind unsuccessfull");
    if(listen(server_fd, 20) == -1)
        perror("FAILED: Listen unsuccessfull");
    return server_fd;
}

string get_room_list(vector<shared_ptr<Room>> &all_rooms) {
    string room_list = "Rooms:\n";
    for (int j = 0; j < all_rooms.size(); ++j) {
        int room_size = all_rooms[j]->get_number_of_clients();
        string available_msg = " (" + to_string(2 - room_size) + " place remind)\n";
        string full_msg = " (Full)\n";
        room_list += "Room " + to_string(j+1) + (room_size < 2 ? available_msg : full_msg);
    }
    return room_list;
}

void handle_room_timer(int signum, siginfo_t *si, void *uc) {
    int timer_id = si->si_value.sival_int;  
    int room_id = room_timer_map[timer_id];
    rooms[room_id]->handle_game(room_id);
    std::string room_list = rooms[room_id]->get_room_list(rooms,room_id);
    rooms[room_id]->send_message_to_all(room_list.c_str());
    rooms[room_id]->send_message_to_all(CHOOSE_ROOM);
    rooms[room_id]->free_room();

    room_timer_map.erase(timer_id);      
    active_timers.erase(timer_id);      
}

void set_room_timer(int room_id, int seconds) {
    struct sigevent sev;
    struct itimerspec its;
    timer_t timer_id;

    current_timer_id++;
    room_timer_map[current_timer_id] = room_id;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;  
    sev.sigev_value.sival_int = current_timer_id;  
    timer_create(CLOCK_REALTIME, &sev, &timer_id);

    its.it_value.tv_sec = seconds;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;  
    its.it_interval.tv_nsec = 0;
    timer_settime(timer_id, 0, &its, NULL);

    active_timers[current_timer_id] = timer_id;
}

void remove_room_timer(int room_id) {
    for (const auto& entry : room_timer_map) {
        if (entry.second == room_id) {
            int timer_id = entry.first;  
            timer_t timer_to_delete = active_timers[timer_id];
            timer_delete(timer_to_delete);
            room_timer_map.erase(timer_id);
            active_timers.erase(timer_id);  
            return;
        }
    }
    cout << "No active timer for Room: " << room_id << endl;
}

void register_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_room_timer;  
    sigemptyset(&sa.sa_mask);
   
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

void broadcast_result(vector<shared_ptr<Player>> clients, struct sockaddr_in bc_address, int bc_sock) {
    std::string message = "\nGame Results:\n";
    for (auto& client : clients) {
        message += client->get_name() + " has " + std::to_string(client->get_wins()) + " wins.\n";
    }

    int result = sendto(bc_sock, message.c_str(), message.size(), 0, (struct sockaddr*)&bc_address, sizeof(bc_address));
    if (result == -1) {
        perror("Broadcast failed");
    }
}

void setup_broadcast(struct sockaddr_in &bc_address, int &bc_sock, int port) {
    int broadcast = 1, opt = 1;
    bc_sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(bc_sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(bc_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(port);
    bc_address.sin_addr.s_addr = inet_addr("127.255.255.255");
    bind(bc_sock, (struct sockaddr *)&bc_address, sizeof(bc_address));
}

void setup_rooms(char* ipaddr, int port, int number_of_rooms, vector<pollfd> &pfds,int bc_sock,struct sockaddr_in bc_address) {
    for (int i = 0; i < number_of_rooms; i++) {
        int room_server_fd = setup_server(ipaddr, port + i + 1);
        pfds.push_back(pollfd{room_server_fd, POLLIN, 0});
        rooms.push_back(make_shared<Room>(room_server_fd, port + i + 1, i + 1,bc_sock,bc_address));
    }
}

void handle_server_commands(vector<pollfd> &pfds, vector<shared_ptr<Player>> &clients,int bc_sock,struct sockaddr_in bc_address) {
    char buffer[1024];
    int bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        std::string command(buffer);
        if (command.find("end_game") != std::string::npos) {
            broadcast_result(clients,bc_address,bc_sock);
            exit(0);
        }
    }
}

void accept_new_client(int server_fd, vector<pollfd> &pfds, vector<shared_ptr<Player>> &clients) {
    struct sockaddr_in new_addr;
    socklen_t new_size = sizeof(new_addr);
    int new_fd = accept(server_fd, (struct sockaddr*)(&new_addr), &new_size);
    write(STDOUT, NEW_CONNECTION, strlen(NEW_CONNECTION));
    pfds.push_back(pollfd{new_fd, POLLIN, 0});
    clients.push_back(make_shared<Player>(new_fd, pfds.size() - 1));
    send_message(new_fd, ASK_NAME);
}

void handle_client_name(vector<pollfd> &pfds, vector<shared_ptr<Player>> &clients, map<int, string> &client_names, size_t i, char* buffer,int client_index) {
    client_names[pfds[i].fd] = string(buffer);
    clients[i - client_index]->set_name(string(buffer));
    string room_list = get_room_list(rooms);
    send_message(pfds[i].fd, room_list.c_str());
    send_message(pfds[i].fd, CHOOSE_ROOM);
}

void handle_room_choice(vector<pollfd> &pfds, vector<shared_ptr<Player>> &clients, size_t i, char* buffer,int client_index) {
    int room_choice = stoi(buffer) - 1;
    if (rooms[room_choice]->get_number_of_clients() >= 2) {
        send_message(pfds[i].fd, ROOM_FULL);
        string room_list = get_room_list(rooms);
        send_message(pfds[i].fd, room_list.c_str());
        send_message(pfds[i].fd, ROOM_FULL);
    } else {
        clients[i - client_index]->set_playing_mode(true, room_choice);
        rooms[room_choice]->add_client(clients[i - client_index]);
        string room_port = "Room: " + to_string(rooms[room_choice]->get_port());
        send_message(pfds[i].fd, room_port.c_str());
    }
}

void handle_existing_connection(vector<pollfd> &pfds, vector<shared_ptr<Player>> &clients, map<int, string> &client_names, size_t i,int client_index) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
    
    if (!clients[i - client_index]->is_get_name()) {
        handle_client_name(pfds, clients, client_names, i, buffer, client_index);
    } else {
        handle_room_choice(pfds, clients, i, buffer, client_index);
    }
}

int main(int argc, char* argv[])
{
    register_signal_handler();
    if(argc != 4)
        perror("Invalid Arguments\n");

    char* ipaddr = argv[1];
    int port = stoi(argv[2]);
    int number_of_rooms = stoi(argv[3]);

    struct sockaddr_in bc_address;
    int bc_sock;
    setup_broadcast(bc_address, bc_sock, port);

    int server_fd = setup_server(ipaddr, port);
    write(1, SERVER_LAUNCHED, strlen(SERVER_LAUNCHED));

    int client_index = 2 + number_of_rooms;
    vector<pollfd> pfds = {pollfd{server_fd, POLLIN, 0}, pollfd{STDIN, POLLIN, 0}};
    setup_rooms(ipaddr, port, number_of_rooms, pfds,bc_sock,bc_address);

    vector<shared_ptr<Player>> clients;
    map<int, string> client_names;

    std::string command_buffer;

    while(true) {
   
        int poll_result = poll(pfds.data(), (nfds_t)(pfds.size()), -1);
        if (poll_result == -1 && errno != EINTR) {
            perror("Poll failed");
            continue;
        }

        for(int i = 0; i < pfds.size(); ++i) {
            bool isClient = i >= client_index;
            if(pfds[i].revents & POLLIN) {
                if(pfds[i].fd == STDIN) {
                    handle_server_commands(pfds, clients,bc_sock,bc_address);
                }
                if(pfds[i].fd == server_fd) {
                    accept_new_client(server_fd, pfds, clients);
                    continue;
                }
                for(int j = 0; j < rooms.size(); ++j) {
                    if(pfds[i].fd == rooms[j]->get_fd()) {
                        struct sockaddr_in new_addr;
                        socklen_t new_size = sizeof(new_addr);
                        int new_fd = accept(rooms[j]->get_fd(), (struct sockaddr*)(&new_addr), &new_size);
                        int change_index = rooms[j]->get_last_client_index();
                        pfds[change_index] = pollfd{new_fd, POLLIN, 0};
                        clients[change_index - client_index]->set_fd(new_fd);

                        if (rooms[j]->get_number_of_clients() == 1) {
                            rooms[j]->send_message_to(ROOM_WAIT, 0);
                        }

                        else if (rooms[j]->get_number_of_clients() == 2) {
                            rooms[j]->send_message_to_all(ROOM_GAME_START);
                            int room_id = j;  
                            set_room_timer(room_id, COUNTDOWN_TIME);
                        }
                    }
                }
                if (isClient) {
                    if (clients[i-client_index]->is_client_playing() == true){
                        int num_room = clients[i-client_index]->get_room();
                        if (rooms[num_room]->get_number_of_clients() <= 1 )
                            continue;
                        char buffer[BUFFER_SIZE];
                        memset(buffer, 0, BUFFER_SIZE);
                        recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                        string ans_str = buffer;
                        int ans = stoi(ans_str);
                        rooms[num_room]->set_ans(pfds[i].fd, ans);
                        if (rooms[num_room]->is_game_end()) {
                            remove_room_timer(num_room);
                            rooms[num_room]->handle_game(num_room);
                            std::string room_list = rooms[num_room]->get_room_list(rooms,num_room);
                            rooms[num_room]->send_message_to_all(room_list.c_str());
                            rooms[num_room]->send_message_to_all(CHOOSE_ROOM);
                            rooms[num_room]->free_room();
                        }
                        continue;
                    }
                }
                if (isClient) {
                    handle_existing_connection(pfds, clients, client_names, i, client_index);
                }
            }
        }
    }
}
