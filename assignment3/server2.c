#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

void send_response(int client_socket, const char *response)
{
    send(client_socket, response, strlen(response), 0);
}

void send_404_response(int client_socket)
{
    const char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
    send_response(client_socket, response);
}

void handle_html_request(int client_socket, const char *requested_resource)
{
    // Check if the requested resource exists as a file
    FILE *file = fopen(requested_resource, "r");
    if (file != NULL)
    {
        // Send file contents
        char response[1024];
        strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), file) != NULL)
        {
            strcat(response, buffer);
        }
        fclose(file);
        send_response(client_socket, response);
    }
    else
    {
        // File not found
        send_404_response(client_socket);
    }
}

void handle_image_request(int client_socket, const char *requested_resource)
{
    // Check if the requested resource exists as a file
    FILE *file = fopen(requested_resource, "rb");
    if (file != NULL)
    {
        // Determine content type based on file extension
        const char *content_type = NULL;
        if (strstr(requested_resource, ".gif") != NULL)
        {
            content_type = "image/gif";
        }
        else if (strstr(requested_resource, ".jpg") != NULL || strstr(requested_resource, ".jpeg") != NULL)
        {
            content_type = "image/jpeg";
        }

        // Send HTTP header with content type
        char response[1024];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
        send_response(client_socket, response);

        // Send image file contents
        char buffer[1024];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
        {
            send(client_socket, buffer, bytes_read, 0);
        }
        fclose(file);
    }
    else
    {
        // File not found
        send_404_response(client_socket);
    }
}

void handle_cgi_script(int client_socket, const char *requested_resource)
{
    // Execute the CGI script as a shell command
    FILE *pipe = popen(requested_resource, "r");
    if (pipe != NULL)
    {
        // Send HTTP header with content type
        const char *content_type = "text/html"; // Assume HTML response by default
        char response[1024];
        sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
        send_response(client_socket, response);

        // Read the output of the CGI script and send it to the client
        char buffer[1024];
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), pipe)) > 0)
        {
            send(client_socket, buffer, bytes_read, 0);
        }

        // Close the pipe
        pclose(pipe);
    }
    else
    {
        // Error executing CGI script
        send_404_response(client_socket);
    }
}

void servConn(int port)
{
    int sd, new_sd;
    struct sockaddr_in name, cli_name;
    int sock_opt_val = 1;
    int cli_len;
    char data[80]; /* Our receive data buffer. */

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("(servConn): socket() error");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val, sizeof(sock_opt_val)) < 0)
    {
        perror("(servConn): Failed to set SO_REUSEADDR on INET socket");
        exit(EXIT_FAILURE);
    }

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sd, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
        perror("(servConn): bind() error");
        exit(EXIT_FAILURE);
    }

    listen(sd, 5);

    for (;;)
    {
        cli_len = sizeof(cli_name);
        new_sd = accept(sd, (struct sockaddr *)&cli_name, &cli_len);
        printf("Assigning new socket descriptor:  %d\n", new_sd);

        if (new_sd < 0)
        {
            perror("(servConn): accept() error");
            continue; // Continue accepting connections
        }

        if (fork() == 0)
        { /* Child process. */
            close(sd);
            memset(data, 0, sizeof(data));
            recv(new_sd, data, sizeof(data), 0); /* Read request from client */
            printf("%s\n", data);

            // Parse HTTP request to extract requested resource
            char http_method[16];         // Buffer to store the HTTP method (e.g., "GET")
            char requested_resource[256]; // Buffer to store the requested directory

            // Parse the request string
            sscanf(data, "%s %s", http_method, requested_resource);

            // Print the extracted directory
            printf("Extracted directory: %s\n", requested_resource);
            printf("method: %s\n", http_method);

            if (strcmp(http_method, "GET") == 0)
            {
                // Handle GET request
                printf("debug: in get, resource= %s\n", requested_resource);

                DIR *dir;
                struct dirent *ent;
                if (strstr(requested_resource, ".html") != NULL)
                {
                    handle_html_request(new_sd, requested_resource);
                }
                else if (strstr(requested_resource, ".gif") != NULL || strstr(requested_resource, ".jpg") != NULL ||
                         strstr(requested_resource, ".jpeg") != NULL)
                {
                    handle_image_request(new_sd, requested_resource);
                }
                else if (strstr(requested_resource, ".cgi") != NULL || strstr(requested_resource, ".py") != NULL)
                {
                    handle_cgi_script(new_sd, requested_resource);
                }
                else if ((dir = opendir(requested_resource)) != NULL) // Check if the requested resource is a directory
                {
                    char response[1024];
                    strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                    while ((ent = readdir(dir)) != NULL)
                    {
                        strcat(response, ent->d_name);
                        strcat(response, "<br>");
                    }
                    closedir(dir);
                    send_response(new_sd, response);
                }
                else
                {
                    // Check if the requested resource exists as a file
                    FILE *file = fopen(requested_resource, "r");
                    if (file != NULL)
                    {
                        // Send file contents
                        char response[1024];
                        strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                        char buffer[1024];
                        while (fgets(buffer, sizeof(buffer), file) != NULL)
                        {
                            strcat(response, buffer);
                        }
                        fclose(file);
                        send_response(new_sd, response);
                    }
                    else
                    {
                        send_404_response(new_sd);
                    }
                }
            }
            else
            {
                // Unsupported HTTP method
                const char *response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
                send_response(new_sd, response);
            }

            close(new_sd); // Close socket before exiting child process
            exit(EXIT_SUCCESS);
        }
        else
        {
            close(new_sd); // Close socket in parent process
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    servConn(atoi(argv[1])); /* Server port. */

    return 0;
}
