#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define PORTNUM 1053
#define MESSAGE_SIZE 70
#define HEARTBEAT_INTERVAL 5 

int server_descriptor;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int heartbeat_count =0 ;
int missed_heartbeat_count =0 ;
time_t last_heartbeat_time =0 ;

void *shm_heartbeat_kick(int);
void shm_heartbeat_cancel();
void shm_restart_application(FILE * , char *);
void shm_terminate_application(FILE * , char *);
void shm_heartbeat_resume(char * , FILE * );
void shm_deregister_client(char * , FILE * );

void *shm_register_application(void *arg)
{
    while (1)
    {
        int application_descriptor;
        struct sockaddr_in application;
        socklen_t address = sizeof(application);

        printf("\n...................The server waiting for the connection...................\n");
        if ((application_descriptor = accept(server_descriptor, (struct sockaddr *)&application, &address)) == -1)
        {
            perror("error at accept");
            exit(1);
        }

        pthread_mutex_lock(&mutex);
        printf("Got connection from application: %s\n", inet_ntoa(application.sin_addr));
        pthread_mutex_unlock(&mutex);

        char message[MESSAGE_SIZE];
        strcpy(message, "Registered To Daemon ..");
        send(application_descriptor, message, MESSAGE_SIZE, 0);

        // Call the heartbeat function
        shm_heartbeat_kick(application_descriptor);

        close(application_descriptor);
    }
    return NULL;
}

void *shm_heartbeat_kick(int application_descriptor)
{
    FILE *file1;
    file1 = fopen("log_file.txt", "a");
    if (file1 == NULL)
    {
        perror("Error opening file");
        exit(1);
    }

    char message[MESSAGE_SIZE];
    int bytes_received;

    while (1)
    {
        // Receive data from the application
        if ((bytes_received = recv(application_descriptor, message, sizeof(message) - 1, 0)) == -1)
        {
            perror("recv");
            exit(1);
        }
        else if (bytes_received == 0)
        {
            printf("Application Disconnected ...\n");
            break; // Exit the loop when the connection is closed
        }

        // Null-terminate the received message
        message[bytes_received] = '\0';

        // Print the received message
        printf("Data received: %s\n", message);

        // Append the message to the file
        if (fprintf(file1, "%s\n", message) < 0)
        {
            perror("Error appending to file");
            fclose(file1);
            exit(1);
        }

        // Flush the file buffer to ensure data is written immediately
        fflush(file1);

        if (strcmp("Cancel" , message)==0)
        {
            shm_heartbeat_cancel();
        }
        else if (strcmp("Resume" , message)==0)
        { 
            shm_heartbeat_resume(message , file1);
        }
        else if (strcmp("Deregister" , message)==0)
        { 
            shm_deregister_client(message , file1);
            break;
        }
        else 
        {
            shm_heartbeat_resume(message , file1);
        }
    }
    fclose(file1);
    return NULL;
}
void shm_heartbeat_cancel()
{
    printf("The deamon stops monitoring the application\n");

}

void shm_heartbeat_resume(char * message  , FILE * file1)
{
    // Calculate time interval between heartbeats
    time_t current_time; // variable to hold the current time 
    time(&current_time); // this time funciton stores the current time in the variable  
    double time_interval = difftime(current_time, last_heartbeat_time); // it finds the diff of the 2 time we have used the difftime function 

    // Update last heartbeat time
    time(&last_heartbeat_time);  // here we are updating the last heartbeat time to the variable 

    // Print time interval
    printf("Time interval between heartbeats: %.2f seconds\n", time_interval);
    sprintf(message , "Time interval between heartbeats: %.2f seconds", time_interval);

    if (fprintf(file1, "%s\n", message) < 0)
        {
            perror("Error appending to file");
            fclose(file1);
            exit(1);
        }

    // Check if time interval is greater than 5 seconds
    if (time_interval <= 5.0)
    {
        printf("Heartbeat received within expected time interval.\n");
        sprintf(message , "Heartbeat received within expected time. \n");

        if (fprintf(file1, "%s\n", message) < 0)
        {
            perror("Error appending to file");
            fclose(file1);
            exit(1);
        }
    }
    else
    {
        printf("Missed heartbeat detected.\n");
        missed_heartbeat_count++;
        printf("Missed heartbeat count: %d\n", missed_heartbeat_count);
        if(missed_heartbeat_count >= 4)
        {
            shm_terminate_application(file1 , message);
        }
    }
}

void shm_deregister_client(char * message , FILE * file1 )
{
    printf("The Deamon Kicked the application from monitoring \n ");
    sprintf(message , "The Deamon Kicked the application from monitoring");

    if (fprintf(file1, "%s\n", message) < 0)
    {
        perror("Error appending to file");
        fclose(file1);
        exit(1);
    }

}
void shm_terminate_application(FILE * file1, char * message)
{
    printf("The missed heartbeat count reached Application restarted  \n ");
    sprintf(message , "The missed heartbeat count reached Application restarted ");

    if (fprintf(file1, "%s\n", message) < 0)
    {
        perror("Error appending to file");
        fclose(file1);
        exit(1);
    }
    int kill_status = system("pkill application_delay");
    if (kill_status == -1) {
        perror("pkill");
        exit(EXIT_FAILURE);
    }
    shm_restart_application(file1 , message);
}
void shm_restart_application(FILE * file1 , char * message)
{

    printf("The Application restarted  \n ");
    sprintf(message , "The Application restarted  ");

    if (fprintf(file1, "%s\n", message) < 0)
    {
        perror("Error appending to file");
        fclose(file1);
        exit(1);
    }
    // Start a new instance of the application
    int start_status = system("xterm -e ./application_delay"); //xterm will create a seperate terminal 
    if (start_status == -1) {
        perror("Starting application failed");
        exit(EXIT_FAILURE);
    }
}

int main()
{
    struct sockaddr_in server;
    pthread_t thread_1, thread_2;

    pid_t process_id;

    server_descriptor = socket(PF_INET, SOCK_STREAM, 0);
    if (server_descriptor == -1)
    {
        perror("Socket creation fails\n");
        exit(1);
    }

    server.sin_family = PF_INET;
    server.sin_port = htons(PORTNUM);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    int yes = 1;
    if (setsockopt(server_descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        perror("setsockopt");
        exit(1);
    }

    if (bind(server_descriptor, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(server_descriptor, 4) == -1)
    {
        perror("error at listen");
        exit(1);
    }

    for (int i = 0; i < 2; i++) {
        process_id = fork();
        if (process_id < 0) {
            perror("fork");
            exit(1);
        }
        if (process_id == 0) {
            printf("Child process created by fork.\n");
            shm_register_application(NULL);
            exit(0);
        }
    }

    pthread_create(&thread_1, NULL, shm_register_application, NULL);
    printf("Thread 1 created by pthread_create.\n");
    pthread_create(&thread_2, NULL, shm_register_application, NULL);
    printf("Thread 2 created by pthread_create.\n");

    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);

    return 0;
}

