#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for getpid()
#include <string.h> // for memset()
#include <errno.h>

#define PORTNUM 1053
#define MESSAGE_SIZE 50
#define HEARTBEAT_INTERVAL 5

int heartbeat_count = 0; // Initialize heartbeat count
int application_descriptor;

void shm_heartbeat_kick();

int main()
{
    char message[MESSAGE_SIZE];

    application_descriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (application_descriptor == -1)
    {
        perror("Socket creation fails\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server;

    memset(&server, 0, sizeof(server)); // Clear server struct
    server.sin_family = PF_INET;
    server.sin_port = htons(PORTNUM);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(application_descriptor, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    int n = recv(application_descriptor, message, MESSAGE_SIZE, 0);
    if (n == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    message[n] = '\0';

    printf("Message received from server is : %s\n", message);

    shm_heartbeat_kick();

    close(application_descriptor);

    return 0;
}

void shm_heartbeat_kick()
{
    pid_t pid = getpid();
    char message[MESSAGE_SIZE];
    int number_of_beat =20 ; 
    while (number_of_beat >= 0 )
    {
        if(number_of_beat >=16 )
        {
            ++heartbeat_count;
            snprintf(message, MESSAGE_SIZE, "Cancel");
            printf("%s\n", message);

            int bytes_sent = send(application_descriptor, message, strlen(message), 0);
            if (bytes_sent == -1)
            {
                perror("send");
                exit(EXIT_FAILURE);
            }
            else if (bytes_sent != strlen(message))
            {
                fprintf(stderr, "Incomplete message sent\n");
                exit(EXIT_FAILURE);
            }

            sleep(3);
        }
        else if(number_of_beat < 16 &&  number_of_beat >=12)
        {
            ++heartbeat_count;
            snprintf(message, MESSAGE_SIZE, "Process %d, Heartbeat count %d", pid, heartbeat_count);
            printf("%s\n", message);

            int bytes_sent = send(application_descriptor, message, strlen(message), 0);
            if (bytes_sent == -1)
            {
                perror("send");
                exit(EXIT_FAILURE);
            }
            else if (bytes_sent != strlen(message))
            {
                fprintf(stderr, "Incomplete message sent\n");
                exit(EXIT_FAILURE);
            }
            heartbeat_count ++ ;

            sleep(5);
        }
        else if (number_of_beat < 12 &&  number_of_beat >=8)
        {
            ++heartbeat_count;
            snprintf(message, MESSAGE_SIZE, "Deregister");
            printf("%s\n", message);

            int bytes_sent = send(application_descriptor, message, strlen(message), 0);
            if (bytes_sent == -1)
            {
                perror("send");
                exit(EXIT_FAILURE);
            }
            else if (bytes_sent != strlen(message))
            {
                fprintf(stderr, "Incomplete message sent\n");
                exit(EXIT_FAILURE);
            }

            sleep(5);
        }
        else if (number_of_beat < 8 &&  number_of_beat >=4)
        {
            ++heartbeat_count;
            snprintf(message, MESSAGE_SIZE, "Process %d, Heartbeat count %d", pid, heartbeat_count);
            printf("%s\n", message);

            int bytes_sent = send(application_descriptor, message, strlen(message), 0);
            if (bytes_sent == -1)
            {
                perror("send");
                exit(EXIT_FAILURE);
            }
            else if (bytes_sent != strlen(message))
            {
                fprintf(stderr, "Incomplete message sent\n");
                exit(EXIT_FAILURE);
            }

            sleep(5);
        }

        number_of_beat --;
    }
}

