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
public:
    Users getUsers(){
        Users users;
        users.push_back(new User(15, "asia"));
        users.push_back(new User(16, "user1"));
        users.push_back(new User(17, "user2"));
        return users;
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

    void storeMessage(int sender, int receiver, std::string body) {
        messages.push_back(new Message(sender, receiver, body));
    }
};

class Response{
public:

    std::string getText(){
        std::stringstream ss;
        std::string body;
        body += this->action + ":";

        for(const auto& s : this->data){
            body+="["+s+"]";
        }

        ss << body.length() + 1 << ":" << body;
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

class Server{
    UsersRepository *usersRepository;
    MessagesRepository *messagesRepository;

public:
    Server(UsersRepository* usersRepository, MessagesRepository* messagesRepository){
        this->usersRepository = usersRepository;
        this->messagesRepository = messagesRepository;
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
            std::string message = (string.substr(pos+1));
            messagesRepository->storeMessage(request->getUser(), receiver_id, message);

            response.setAction("OK");
        }
        return response;
    }

};

int getLength(int desc){
    char buf[5];
    int read_result = static_cast<int>(read(desc, buf, sizeof(buf))); 
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
    auto *pServer = new Server(usersRepository, pMessagesRepository);

    //dostÄp do pĂłl struktury: (*th_data).pole

    printf("Nowe połączenie");
    while (1) {
        int length = getLength(th_data->connection_socket_descriptor);
        if (length != -1) {
            char message[1000];
            for (int i = 0; i < 1000; ++i) {
                message[i] = 0;
            }
            int read_result = static_cast<int>(read(th_data->connection_socket_descriptor, message, length !=0 ? length : sizeof(message)));
            std::string m(message);
            const std::string &buf = pServer->process(new Request(15, m)).getText();

            write(th_data->connection_socket_descriptor, buf.c_str(), buf.length());
            printf(message);
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