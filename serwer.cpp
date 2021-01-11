#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>


#define SERVER_PORT 1232
#define QUEUE_SIZE 5


//struktura zawierajÄca dane, ktĂłre zostanÄ przekazane do wÄtku
struct thread_data_t
{
    int connection_socket_descriptor;
};

class User{
    int user_id;
    std::string user_name;
    
public:
    User(int user_fd, std::string user_name){
        this->user_id = user_fd;
        this->user_name = std::move(user_name);
    }

    std::string getUserName() {
        return this->user_name;
    }

    int getUserId(){
        return this->user_id;
    }
};

typedef std::vector<User *> Users;

class UsersRepository{
    Users users;
public:
    Users getUsers(){
        return users;
    }

    void storeUser(User* user){
        users.push_back(user);
    }

    void removeUser(int user){
        Users result;
        std::copy(users.begin(), users.end(), std::back_inserter(result));
        users.clear();
        std::copy_if (result.begin(), result.end(), std::back_inserter(users), [user](User* u) {
            return (u->getUserId() != user);
        });
    }
};

class Message{
    int sender_id;
    int receiver_id;
    std::string text;

public:
    Message(int sender_id, int receiver_id, std::string text){
        this->sender_id = sender_id;
        this->receiver_id = receiver_id;
        this->text = std::move(text);
    }

    std::string getMessage() {
        return this->text;
    }

    int getSenderId(){
        return this->sender_id;
    }

    int getReceiverId(){
        return this->receiver_id;
    }
};

typedef std::vector<Message *> Messages;

class MessagesRepository{
    Messages messages;
public:
    Messages getMessages(){
        messages.push_back(new Message(15, 16, "asia"));
        messages.push_back(new Message(16, 15, "asiaasia"));
        messages.push_back(new Message(15, 16, "asia2"));
        messages.push_back(new Message(15, 17, "asia3"));
        return messages;
    }

    Messages getUsersMessages(int user1, int user2){
        const Messages &messages = getMessages();
        Messages result;
        std::copy_if (messages.begin(), messages.end(), std::back_inserter(result), [user2, user1](Message* m){
            return (m->getReceiverId() == user1
                    && m->getSenderId() == user2)
                    ||
                    (m->getReceiverId() == user2
                     && m->getSenderId() == user1)
                    ;
        });

        return result;
    }

    void storeMessage(Message* message) {
        messages.push_back(message);
    }
};

class Response{
    std::string getLengthInfo(int length, int bytes=5){
        std::string l = std::to_string(length);
        l += ':';
        unsigned long zeros_to_begin = bytes - l.size();
        for (int i = 0; i < zeros_to_begin; ++i) {
            l.insert(0, "0");
        }
        return l;
    }

public:
    std::stringstream  ss;
    std::string getText(){
        std::stringstream ss;
        std::string body;
        body += this->action + ":";

        for(const auto& s : this->data){
            body+=getLengthInfo(s.size(), 4) + s;
        }

        ss << getLengthInfo(body.length(), 5) << body;
        return ss.str() ;
    }

    void setAction(std::string action_name){
        this->action = std::move(action_name);
    }

    void setData(const std::string& datum) {
        this->data.push_back(datum);
    }

private:
    std::string action;
    std::vector<std::string> data;
};

class Request{
    std::vector<std::string> parts;
    int user;
public:
    Request(int user, std::string data){
        this->user = user;
        std::string delimiter = ":";
        size_t pos = 0;
        std::string token;
        pos = data.find(delimiter);
        token = data.substr(0, pos);
        parts.push_back(token);
        data.erase(0, pos + delimiter.length());
        parts.push_back(data);
    }
    
    std::string getAction(){
        return parts[0];
    };

    std::string getData(){
        return parts[1];
    }

    int getUser(){
        return user;
    }

};

class Emitter{
public:
    void emitMessageToReceiver(Message* message){
        Response response;
        response.setAction("newMessage");
        response.ss << message->getSenderId() << "," << message->getMessage();
        response.setData(response.ss.str());
        write(message->getReceiverId(), response.getText().c_str(), response.getText().length());
    }

    void emitActionToUsers(Users users, std::string action, std::string data) {
        Response response;
        response.setAction(action);
        response.setData(data);
        for (auto* u : users) {
            write(u->getUserId(), response.getText().c_str(), response.getText().length());
        }
    }

    void emitActionToUser(User* user, std::string action, const std::string& data) {
        Response response;
        response.setAction(std::move(action));
        response.setData(data);
        write(user->getUserId(), response.getText().c_str(), response.getText().length());
    }
    
    void emitResponse(User* user, Response* response){
        write(user->getUserId(), response->getText().c_str(), response->getText().size());
    }


};

class Server{
    UsersRepository *usersRepository;
    MessagesRepository *messagesRepository;
    Emitter* emitter;
public:
    Server(UsersRepository* usersRepository, MessagesRepository* messagesRepository, Emitter* emitter){
        this->usersRepository = usersRepository;
        this->messagesRepository = messagesRepository;
        this->emitter = emitter;
    }

    Response process(Request* request){
        Response r;
        try {
            return _process(request);
        } catch (...) {
            r.setAction("invalid");
            return r;
        }
    }

    Response _process(Request* request){
        Response response;
        if(request->getAction() == "getUsers") {
            response.setAction("users");
            for (auto *user: usersRepository->getUsers()) {
                std::stringstream ss;
                ss << user->getUserName() << "," << user->getUserId();
                response.setData(ss.str());
            }
        }else if(request->getAction() == "getMessages") {
            response.setAction("messages");
            int user2 = std::stoi(request->getData());
            for (auto *message: messagesRepository->getUsersMessages(request->getUser(), user2)) {
                std::stringstream ss;
                ss << message->getSenderId() << "," << message->getMessage();
                response.setData(ss.str());
            }
        } else if (request->getAction() == "sendMessage"){
            const std::string &string = request->getData();
            size_t pos = string.find(',');
            int receiver_id  = stoi(string.substr(0, pos));
            std::string message_body = (string.substr(pos+1));
            auto * message = new Message(request->getUser(), receiver_id, message_body);
            messagesRepository->storeMessage(message);
            emitter->emitMessageToReceiver(message);
            response.setAction("OK");
        }
        return response;
    }

    void removeUser(int user){
        usersRepository->removeUser(user);
        emitter->emitActionToUsers(usersRepository->getUsers(), "logoutUser", std::to_string(user));
    }

    void loginUser(User* user){
        emitter->emitActionToUsers(usersRepository->getUsers(), "loginUser", std::to_string(user->getUserId()));
        emitter->emitActionToUser(user, "your_id", std::to_string(user->getUserId()));
        usersRepository->storeUser(user);
        Response response = process(new Request(user->getUserId(), "getUsers"));
        emitter->emitResponse(user, &response);
    }
};

int getLength(int desc){
    char buf[5];
    int read_result = static_cast<int>(read(desc, buf, sizeof(buf)));
    if(read_result == 0){
        return -2;
    }
    if (buf[4] != ':'){
        return -1;
    }
    buf[4]=0;
    return atoi(buf);
}

MessagesRepository *pMessagesRepository = new MessagesRepository();
UsersRepository *usersRepository = new UsersRepository();


//funkcja opisujÄcÄ zachowanie wÄtku - musi przyjmowaÄ argument typu (void *) i zwracaÄ (void *)
void *ThreadBehavior(void *t_data)
{
    pthread_detach(pthread_self());

    auto *th_data = (struct thread_data_t*)t_data;
    auto *pServer = new Server(usersRepository, pMessagesRepository, new Emitter);

    pServer->loginUser(new User(th_data->connection_socket_descriptor, "user"));
    //dostÄp do pĂłl struktury: (*th_data).pole

    printf("Nowe połączenie: %d\n", th_data->connection_socket_descriptor);
    while (1) {
        int length = getLength(th_data->connection_socket_descriptor);
        if (length > -1) {
            char message[1000];
            for (int i = 0; i < 1000; ++i) {
                message[i] = 0;
            }
            int read_result = static_cast<int>(read(th_data->connection_socket_descriptor, message, length !=0 ? length : sizeof(message)));
            std::string m(message);
            const std::string &buf = pServer->process(new Request(th_data->connection_socket_descriptor, m)).getText();

            write(th_data->connection_socket_descriptor, buf.c_str(), buf.length());
            printf(message);
        }else if(length == -2){
            printf("zerwano");
            pServer->removeUser(th_data->connection_socket_descriptor);
            break;
        }
    }

    pthread_exit(NULL);
}

//funkcja obsĹugujÄca poĹÄczenie z nowym klientem
void handleConnection(int connection_socket_descriptor) {
    //wynik funkcji tworzÄcej wÄtek
    int create_result = 0;

    //uchwyt na wÄtek
    pthread_t thread1;

    //dane, ktĂłre zostanÄ przekazane do wÄtku
    //TODO dynamiczne utworzenie instancji struktury thread_data_t o nazwie t_data (+ w odpowiednim miejscu zwolnienie pamiÄci)
    //TODO wypeĹnienie pĂłl struktury
    auto *t_data = new thread_data_t;
    memset(t_data, 0, sizeof(thread_data_t));
    t_data->connection_socket_descriptor = connection_socket_descriptor;

    create_result = pthread_create(&thread1, NULL, ThreadBehavior, (void *)t_data);
    if (create_result){
        printf("Błąd przy próbbie utworzenia wątku, kod błędu: %d\n", create_result);
        exit(-1);
    }

    //TODO (przy zadaniu 1) odbieranie -> wyĹwietlanie albo klawiatura -> wysyĹanie
}

int main(int argc, char* argv[])
{
    int server_socket_descriptor;
    int connection_socket_descriptor;
    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;

    //inicjalizacja gniazda serwera

    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);

    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_descriptor < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbbie utworzenia gniazda..\n", argv[0]);
        exit(1);
    }
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

    bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if (bind_result < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbbie dowiązzania adresu IP i numeru portu do gniazda.\n", argv[0]);
        exit(1);
    }

    listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
    if (listen_result < 0) {
        fprintf(stderr, "%s: Błąd przy próbbie ustawienia wielkości kolejki.\n", argv[0]);
        exit(1);
    }

    while(1)
    {
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
        if (connection_socket_descriptor < 0)
        {
            fprintf(stderr, "%s: Błąd przy próbbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
        }

        handleConnection(connection_socket_descriptor);
    }

    close(server_socket_descriptor);
    return(0);
}