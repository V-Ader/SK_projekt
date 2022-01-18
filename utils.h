enum type{login = 0, login_response = 1, add_to_queue = 2, get_queue_size = 3, get_queue_item = 4,
 response = 10};

#define PROVIDERS_SIZE 5
#define CLIENTS_SIZE 5

typedef struct package {
        int pg_type;    //0 - 99
        int size;       //0 - 99
        char text[196];  
}Package;
 
typedef struct queue {
        int size;                       //elements_counter 
        int max_size;                   //max items number
        Package list[100];              //list of elements
}Queue;

void queueInsert(Queue *q, Package p){
        if(q->size >= 99){
                return;
        }
        
        int i = 0;
        for(i = q->size; i >= 1; i--){
                q->list[i+1] = q->list[i];
        }

        q->list[1] = p;
        q->size++;
}

Package queueGetItem(Queue *q){
        Package ans = q->list[q->size];
        q->size--;

        return ans;
}


typedef struct client {
        char name[10];
        Queue queue[100];

}Client;

typedef struct provider {
        int id;
        char name[10];
        Client* list_of_clients[10];
}Provider;

void send_to_clients(Provider *provider, Package *package){
        int i;
        for(i = 0; i < 10; i++){
                if(provider->list_of_clients[i] != NULL){
                        queueInsert(provider->list_of_clients[i]->queue, *package);
                }
        }

}

void serialize_to_char(Package *p, char *list){
        list[0] = p->pg_type/10 + '0';
        list[1] = p->pg_type%10 + '0';
        list[2] = p->size/10 + '0';
        list[3] = p->size%10 + '0';
        int i;
        for(i = 0; i < p->size; i++){
                list[i+4] = p->text[i];
        }
        list[i+4] = '\0';
}

void deserialize_from_char(char *list, Package *p){
        p->pg_type = 0;
        p->pg_type += (list[0] - '0')*10;
        p->pg_type += (list[1] - '0');
        p->size = 0;
        p->size += (list[2] - '0')*10;
        p->size += (list[3] - '0');
        

        int i;
        for(i = 0; i < p->size; i++){
                p->text[i] = list[i+4];
        }
        p->text[p->size] = '\0';

}


