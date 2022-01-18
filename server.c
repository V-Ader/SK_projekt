#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "utils.h"

#define BUF_SIZE 200


Provider providers[PROVIDERS_SIZE];
Client clients[CLIENTS_SIZE];

/*
this function sets data for testing purpose
*/
void set_providers(){
    printf("create users\n");
    memcpy(clients[0].name, "Karol", strlen("Karol"));
    memcpy(clients[1].name, "Beata", strlen("Beata"));

    printf("create prviders\n");
    memcpy(providers[0].name, "Alpha", strlen("Alpha"));
    providers[0].list_of_clients[0] = &clients[0];

    memcpy(providers[1].name, "Beta", strlen("Beta"));
    providers[1].list_of_clients[0] = &clients[0];
    providers[1].list_of_clients[1] = &clients[1];

    memcpy(providers[2].name, "Gamma", strlen("Gamma"));

    Package test_alpha = {response, strlen("Alpha: wiadomosc 1"), "Alpha: wiadomosc 1"};
    send_to_clients(&providers[0], &test_alpha);

    Package test_beta = {response, strlen("Beta: wiadomosc 2"), "Beta: wiadomosc 2"};
    send_to_clients(&providers[1], &test_beta);

    Package test_gamma = {response, strlen("Gamma: wiadomosc 3"), "Gamma: wiadomosc 3"};
    send_to_clients(&providers[2], &test_gamma);
}

struct cln {
        int cfd;
        struct sockaddr_in caddr;
};

void* cthread(void* arg) {
    struct cln* c = (struct cln*)arg;
    printf("New connection: %s\n",inet_ntoa((struct in_addr)c->caddr.sin_addr));
    char buffer[BUF_SIZE];
    char buffer_out[BUF_SIZE];
    Package package_recived; 
    Package package_send; 
    Client *current_clinet;
    Provider *current_provider;

    int login_success = 0;
    //red firs package. It must be login type.
    read(c->cfd, &buffer, BUF_SIZE);
    deserialize_from_char(buffer, &package_recived);
    if(package_recived.pg_type == login){
        
        //authorization:
        printf("authorization...\n");
        int i;
        for(i = 0; i < CLIENTS_SIZE; ++i){
            printf("is %s -> %s? \n",package_recived.text, clients[i].name );
            if(strcmp(clients[i].name, package_recived.text) == 0){
                current_clinet = &clients[i];
                current_provider = NULL;
                login_success = 1;
                break;
            }
        }//if there is no result in clients, then check providers
        if(login_success != 1){
            for(i = 0; i < PROVIDERS_SIZE; ++i){
                printf("is %s -> %s? \n",package_recived.text, providers[i].name );
                if(strcmp(providers[i].name, package_recived.text) == 0){
                    current_provider = &providers[i];
                    current_clinet = NULL;
                    login_success = 1;
                    break;
                }   
            }
        }
    }

    //unsuccessful case 
    if(login_success == 0){
        printf("Wrong username/provedername\n");
        package_send.pg_type = login_response;
        memcpy(package_send.text, "Wrong username/provedername", strlen("Wrong username/provedername"));
        package_send.size = strlen("Wrong username/provedername");

        serialize_to_char(&package_send, buffer_out);
        write(c->cfd,buffer_out, sizeof(buffer_out));

        close(c->cfd);
        free(c);
        return EXIT_SUCCESS;
    }

    
    //its time to inform the client if he is a client or provider

    package_send.pg_type = login_response;
    if(current_clinet == NULL){
        printf("Welcome provider %s...\n", current_provider->name);
        memcpy(package_send.text, "provider", strlen("provider"));
        package_send.size = strlen("provider");
    }
    else{
        printf("Welcome client %s...\n", current_clinet->name);
        memcpy(package_send.text, "client", strlen("client"));
        package_send.size = strlen("client");
    }

    serialize_to_char(&package_send, buffer_out);
    write(c->cfd,buffer_out, sizeof(buffer_out));


    // then wait for orders from client
    while (read(c->cfd, &buffer, BUF_SIZE) > 0) {
        deserialize_from_char(buffer, &package_recived);
        printf("recived: id: %d, size: %d, value: %s\n", package_recived.pg_type, package_recived.size, package_recived.text);

        if(package_recived.pg_type == get_queue_size){
            package_send.pg_type = response;
            package_send.text[0] = current_clinet->queue->size/10 + '0';
            package_send.text[1] = current_clinet->queue->size%10 + '0';
            package_send.text[2] = '\0';
            package_send.size = 2;

            printf("    item to send: %s\n", package_send.text);

            serialize_to_char(&package_send, buffer_out);
            write(c->cfd,buffer_out, sizeof(buffer_out));
        }

        if(package_recived.pg_type == get_queue_item){
            package_send = queueGetItem(current_clinet->queue);
            package_send.pg_type = response;
            package_send.size = strlen(package_send.text);

            printf("    item to send: %s\n", package_send.text);

            serialize_to_char(&package_send, buffer_out);
            write(c->cfd,buffer_out, sizeof(buffer_out));
        }
        
        if(package_recived.pg_type == add_to_queue){
            send_to_clients(current_provider, &package_recived);
            package_send.pg_type = response;
            memcpy(package_send.text, "done", strlen("done"));
            package_send.size = strlen("done");

            serialize_to_char(&package_send, buffer_out);
            write(c->cfd,buffer_out, sizeof(buffer_out));
        }
        
    }
    sleep(100);
    close(c->cfd);
    free(c);
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    set_providers();
    pthread_t tid;
    socklen_t slt;
    int sfd, on = 1;
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(1234);
    sfd = socket(AF_INET,SOCK_STREAM,0);
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
    bind(sfd,(struct sockaddr*)&saddr, sizeof(saddr));
    listen(sfd, 10);
    while(1) {
        struct cln* c = malloc(sizeof(struct cln));
        slt = sizeof(c->caddr);
        c->cfd = accept(sfd, (struct sockaddr*)&c->caddr, &slt);
        pthread_create(&tid, NULL, cthread, c);
        pthread_detach(tid);
    }
    close(sfd);
    return 0;
}