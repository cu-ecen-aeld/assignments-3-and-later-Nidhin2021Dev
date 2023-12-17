#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <linux/fs.h>


#define BACKLOG 10
#define BUFFER_SIZE 1024  
#define PORT "9000"  

const char *DATA_FILE = "/var/tmp/aesdsocketdata";


typedef struct threads{
    pthread_t ids;
    int client_fd;
    bool complete;
    TAILQ_ENTRY(threads) nodes;
}node_t;

typedef TAILQ_HEAD(head_s, threads) head_t;
head_t head;


static void signal_handler(int signo);
void *thread_func(void* thread_param);
void *cleanup(void* clean_args);
void *timestamp_func(void *timestamp);
void free_queue(head_t * head);
node_t* _fill_queue(head_t * head, const pthread_t threadid, const int threadclientid);


int count_nodes, count_nodes1;
int sock_fd , newsock_fd , fd ;
int status , sockstat , listenstat ,  thread_ret ,bindstat;


static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t timestampthread;
pthread_t threadc;


int main(int argc, char *argv[])
{
    struct addrinfo hints; 
    struct addrinfo *servinfo = NULL;
    struct sockaddr_in store;
    socklen_t addr_size; 
    
    // Create a socket
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    char ipaddress[INET_ADDRSTRLEN];
    
    memset(ipaddress, 0, sizeof ipaddress);
    
    int yes = 1;
    
    // Open log
    openlog(NULL, LOG_CONS, LOG_USER);
    
    // Register signal_handler
    if(signal(SIGINT, signal_handler) == SIG_ERR)
    {
        syslog(LOG_ERR,"SIGINT");
        closelog();
    }
    if(signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        syslog(LOG_ERR,"SIGTERM");
        closelog();
    }
    if(signal(SIGKILL, signal_handler) == SIG_ERR)
    {
        syslog(LOG_ERR,"SIGKILL");
        closelog();
    }
    
    // Returns addrinfo structures
    if((status = getaddrinfo(NULL, PORT, &hints, &servinfo))!=0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n",gai_strerror(status));
        closelog();
        return -1;
    }

    // Obtain socket file file descriptor
    sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sock_fd == -1)
    {
        perror("Server: socket");
        syslog(LOG_ERR,"Server: socket");
        closelog();
        return -1;
    }   
    
    // Avoid the already in use message
    sockstat = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if(sockstat == -1)
    {
        perror("Server: setsockopt");
        syslog(LOG_ERR,"Server: setsockopt");
        close(sock_fd);
        closelog();
        return -1;
    }
    
    // Bind socket to port and assign an address
    bindstat = bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(bindstat == -1)
    {
        perror("Server: bind");
        syslog(LOG_ERR,"Server: bind");
        close(sock_fd);
        closelog();
        return -1;
    }
    
    remove(DATA_FILE);
    
    // Check if -d argument is present and fork daemon
    if(bindstat == 0)
    {
        
        if(argc ==2 && (!strcmp(argv[1],"-d")))
        {
            daemon(0,0);
 
        }
    }
    
    freeaddrinfo(servinfo);

    // Listen for incoming connections
    listenstat= listen(sock_fd, BACKLOG);
    if(listenstat == -1)
    {
        perror("Server: listen");
        syslog(LOG_ERR,"Server: listen");
        closelog();
        close(sock_fd);
        return -1;
    }
    
    TAILQ_INIT(&head);
    
    // Clean pthread
    int thread_clean = pthread_create(&threadc, NULL, cleanup, NULL);
    if(thread_clean!= 0)
    {
            perror("pthread_create cleanup");
            syslog(LOG_ERR,"pthread_create cleanup");
            return -1;
    }
     
    // Create timestamp  
    int timestampret = pthread_create(&timestampthread, NULL, timestamp_func, NULL);
    if(timestampret != 0){
        perror("pthread_create timestamp");
        syslog(LOG_ERR, "Error: Timestamp thread creation failed: %s", strerror(errno));
        return -1;
    }

    while(1)
    {
    	// Accept incoming connection
        addr_size = sizeof(store);
        newsock_fd = accept(sock_fd, (struct sockaddr *)&store, &addr_size);
        if(newsock_fd == -1)
        {
            perror("Server: accept");
            syslog(LOG_ERR,"Server: accept");
            raise(SIGTERM);
            return -1;
        }
        
        // Get address of the connected client
        if(store.sin_family == AF_INET)
        {
            inet_ntop(AF_INET, &store.sin_addr, ipaddress, INET_ADDRSTRLEN);
            syslog(LOG_DEBUG,"Accepted connection from %s",ipaddress);
        }
        
        node_t *current = NULL;
        current = _fill_queue(&head, 0, newsock_fd);
        
        // Create new thread for the accepted connection
        thread_ret = pthread_create(&(current->ids), NULL, thread_func, (void *)current);
        if(thread_ret)
        {
                perror("pthread_create");
                exit(EXIT_FAILURE);
        }
        
    }     
    closelog();
    close(sock_fd); 
    close(newsock_fd);
    return 0;
}   


void *thread_func(void* thread_param)
{
    struct threads *thread_func_args = thread_param;
    
    int bytesread, ret;        
            
    char buf[BUFFER_SIZE];
    char send_buf[BUFFER_SIZE];
    
    memset(buf,0,BUFFER_SIZE);
    memset(send_buf,0,BUFFER_SIZE);
    
    //Mutex lock
    pthread_mutex_lock(&mutex);
    fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0777);
    if(fd < 0)
    {
        closelog();
    } 
    
    
    while(1)
    {
        // Receive data packets
        bytesread = recv(thread_func_args->client_fd, buf, sizeof(buf), 0);
        if(bytesread == -1)
        {
            pthread_mutex_unlock(&mutex);
            perror("Server: recv");
            syslog(LOG_ERR,"Server: recv");
            pthread_exit(NULL);
        }
          
        // Append data to open file
        ret = write(fd, buf, bytesread);
        if(ret == -1)
        {
            pthread_mutex_unlock(&mutex);
            perror("Write failed");
            syslog(LOG_ERR,"Write failed");
        }
        
        if(buf[bytesread-1] == '\n')
            break;
            
    }
    
    lseek(fd, 0, SEEK_SET);
    while(1)
    {
        ret = read(fd, send_buf, BUFFER_SIZE); 
        if(ret<0)
        {
            perror("Read failed");
            syslog(LOG_ERR,"Read failed");
            pthread_exit(NULL);
        }
        
        // Send response of data from file
        int bytes_sent = send(thread_func_args->client_fd, send_buf, ret, 0);
        if(bytes_sent == -1)
        {
            perror("Server: send");
            syslog(LOG_ERR,"Server: send");
            pthread_exit(NULL);
        }
        
        
        if(!ret)
        {
            break;
        }
    }

    thread_func_args->complete = 1;
    close(fd);
    shutdown(thread_func_args->client_fd, SHUT_RDWR);
    pthread_mutex_unlock(&mutex);
    return NULL;
}


void *cleanup(void* clean_args)
{
    while(1)
    {
        node_t *tmp = NULL;
        TAILQ_FOREACH(tmp, &head, nodes)
        {
            if(tmp->complete == 1)
            {
                syslog(LOG_DEBUG,"Cleaning thread %ld with complete value %d", tmp->ids, tmp->complete);
                
                if((pthread_join(tmp->ids, NULL))!=0)
                {
                    syslog(LOG_ERR,"pthread_join failed from cleanup");
                }
                TAILQ_REMOVE(&head, tmp, nodes);
                free(tmp);
                tmp = NULL;
                count_nodes1++;
                break;    
            }
        }
        usleep(10);
    }       
    return NULL;        
}


void *timestamp_func(void *timestamp)
{
    while(1)
    {
       char outstr[200] ={0};
           time_t t;
           struct tm *tmp;
           
           t = time(NULL);
           tmp = localtime(&t);
           if (tmp == NULL) {
               perror("localtime");
           }

           if (strftime(outstr, sizeof(outstr),"timestamp: %a %d %b %Y %T", tmp) == 0) 
           {
               perror("strftime error");
               syslog(LOG_ERR,"strftime error");
           }
           
           strcat(outstr,"\n");

        // Mutex lock
        pthread_mutex_lock(&mutex);
        
        fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0777);
        if(fd < 0)
        {
            printf("File not created for timestamp\n");
        }
        else
        {
            printf("File created successfully in append mode for timestamp\n");
        }   

        // Write data to the file opened
        int stampret = write(fd, outstr, strlen(outstr));
        if(stampret == -1)
        {
            perror("Write failed");
            syslog(LOG_ERR,"Write failed");
        }
        close(fd);
        
        // Mutex unlock
        pthread_mutex_unlock(&mutex);   

        sleep(10);
    }

}

    
// Signal handler for SIGINT and SIGTERM
static void signal_handler(int signo)
{
        syslog(LOG_DEBUG,"Caught signal, exiting");

        node_t *handle = NULL;
        TAILQ_FOREACH(handle, &head, nodes)
        {
            if((shutdown(handle->client_fd, SHUT_RDWR)) == -1)
            {
            	syslog(LOG_ERR,"Shutdown failure from threads");
            }
            
            if((pthread_kill(handle->ids, SIGKILL))!=0)
            {
            	syslog(LOG_ERR,"Unable to kill thread");
            }
            
        }
        
        free_queue(&head);
        pthread_mutex_destroy(&mutex);
        
        syslog(LOG_DEBUG,"Mutex destroyed");
        
        if(shutdown(sock_fd, SHUT_RDWR) == -1)
        {
        	perror("Signal Handler : Shutdown");
        	syslog(LOG_ERR,"Shutdown failure");
        	closelog();
        }
        
        if((pthread_kill(threadc,SIGKILL)!=0))
        {
        	syslog(LOG_ERR,"threadc FAILURE");
        }
        
        if((pthread_kill(timestampthread,SIGKILL))!=0)
        {
        	syslog(LOG_ERR,"timestampthread FAILURE");
        }
        
        if(sock_fd)
        {
        	close(sock_fd);
        	syslog(LOG_ERR,"sock_fd FAILURE");   
        }
        
        if(newsock_fd)
        {
		close(newsock_fd);
		syslog(LOG_ERR,"newsock_fd FAILURE");
        }
        
        if(fd)
        {
        	close(fd);
        	syslog(LOG_ERR,"fd FAILURE");
        }
        
        unlink(DATA_FILE);
        closelog();
}


// Manages the linked list of given threads
node_t* _fill_queue(head_t * head, const pthread_t threadid, const int threadclientid)
{
    struct threads *e = malloc(sizeof(struct threads));
    if(e == NULL)
    {
        printf("Malloc Unsuccessfull\n");
        exit(EXIT_FAILURE);
    }
    e->ids = threadid;
    e->client_fd = threadclientid;
    e->complete = 0;
    TAILQ_INSERT_TAIL(head,e,nodes);
    e = NULL;

    count_nodes++;

    return TAILQ_LAST(head, head_s);
}


// Removes all elements from the queue
void free_queue(head_t * head)
{
    struct threads *e = NULL;
    while(!TAILQ_EMPTY(head))
    {
        e = TAILQ_FIRST(head);
        TAILQ_REMOVE(head,e,nodes);
        free(e);
        e = NULL;
    }
}
