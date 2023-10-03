#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

#define LOGGING_CLIENT_DATA_PATH "/var/tmp/aesdsocketdata"

int socket_fd = 0, client_fd = 0;
const int max_connection = 10;
struct sockaddr_in addr;
int addr_size = sizeof(struct sockaddr_in);
char client_addr[INET_ADDRSTRLEN];
int data_buffer_size = sizeof(char) * 512;
char *data_buffer;
bool daemon_b = false;



int read_from_aesdsockdata(char *data, int *buffer_size)
{
	int fd = open(LOGGING_CLIENT_DATA_PATH, O_RDWR|O_APPEND|O_CREAT, S_IRWXO|S_IRWXG|S_IRWXU);
	
	if (fd == -1)
	{
		exit(-1);
	}
	
	int counter = 0;
	int res = -1;
	
	
	while((res = read(fd, &data[counter], 1)) == 1)
    {
        if (res == -1)
        {
            exit(-1);
        }

        if (data_buffer[counter] == '\0')
        {
            break;
        }

        counter++;

        if ((counter) >= *buffer_size)
        {
            *buffer_size += 512;
            data = realloc(data, *buffer_size);
        }
    }
	
	close(fd);
	
	return counter;
	
}

void write_to_aesdsockdata(char *data, const int len)
{
	int fd = open(LOGGING_CLIENT_DATA_PATH, O_RDWR|O_APPEND|O_CREAT, S_IRWXO|S_IRWXG|S_IRWXU);
	
	if (fd == -1)
	{
		exit(-1);
	}
	
	write(fd, data_buffer, len);
	
	close(fd);
}

void before_exit()
{
	if (data_buffer != NULL)
        free(data_buffer);

    if (client_fd == -1 || client_fd == 0)
    {
        close(client_fd);
    }

    if (socket_fd == -1 || socket_fd == 0)
    {
        shutdown(socket_fd, SHUT_RDWR); 
    }
	
	closelog();
	
	remove(LOGGING_CLIENT_DATA_PATH);
	
}

void hande_signal()
{
    syslog(LOG_USER, "Caught signal, exiting");
    exit(0);
}


int main(int argc, char **argv)
{
	
	for (int i = 0; i < argc; i++)
	{
		
		if (strcmp(argv[i], "-d") == 0)
		{
			daemon_b = true;
		}
	}

    openlog("aesdsocket", 0, LOG_USER);
    
    data_buffer = (char *) malloc(data_buffer_size);
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    addr = (struct sockaddr_in){.sin_family = AF_INET, .sin_port = htons(9000), .sin_addr = (struct in_addr){.s_addr = INADDR_ANY}};

    signal(SIGTERM, hande_signal);
    signal(SIGINT, hande_signal);
	
	int option = 1;
	
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
	{
		exit(-1);
	}
	
	if (atexit(&before_exit))
	{
		exit(-1);
	}

    if (socket_fd == -1)
    {
        exit(-1);
    }


    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)))
    {
        exit(-1);
    }

    if (listen(socket_fd, max_connection) == -1)
    {
        exit(-1);
    }
	
	if (daemon_b)
	{
		pid_t pid = fork();

		if (pid != 0)
		{
			exit(0);
		}
	}

	while (1)
	{
		
		client_fd = accept(socket_fd, (struct sockaddr *) &addr, &addr_size);
		
		if (client_fd == -1)
		{
			exit(-1);
		}


		inet_ntop(AF_INET, &addr.sin_addr, client_addr, INET_ADDRSTRLEN);

		syslog(LOG_USER, "Accepted connection from %s", client_addr);

		int counter = 0;
		int res;

		while((res = read(client_fd, &data_buffer[counter], 1)) == 1)
		{
			if (res == -1)
			{
				hande_signal();
			}

			if (data_buffer[counter] == '\n' || data_buffer[counter] == '\0')
			{
				break;
			}

			counter++;

			if ((counter) >= data_buffer_size)
			{
				data_buffer_size += 512;
				data_buffer = realloc(data_buffer, data_buffer_size);
			}
		}

		if ((counter + 2) >= data_buffer_size)
		{
			data_buffer_size += 512;
			data_buffer = realloc(data_buffer, data_buffer_size);
		}

		char *newline = strchr(data_buffer, '\n');

		*(newline + 1) = '\0';

		
		
		write_to_aesdsockdata(data_buffer, (newline-data_buffer) + 1);
		
		int data_to_send = read_from_aesdsockdata(data_buffer, &data_buffer_size);
		
		write(client_fd, data_buffer, data_to_send);
		
		close(client_fd);
	}


	
	return -1;

}
