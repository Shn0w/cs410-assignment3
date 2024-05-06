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
        char buffer[5000];
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

void histogram(int client_socket, char *address)
{
    char tmp[100];
    sprintf(tmp, "/usr/bin/python3 my-histogram.py %s", address);
    system(tmp);
    FILE *image_file = fopen("/home/daniel/Desktop/assignment3/histogram.jpg", "rb");
    if (!image_file)
    {
        perror("Error opening image file");
        return;
    }

    fseek(image_file, 0, SEEK_END);
    long image_size = ftell(image_file);
    rewind(image_file);

    char *image_buffer = (char *)malloc(image_size);
    if (!image_buffer)
    {
        perror("Error allocating memory for image buffer");
        fclose(image_file);
        return;
    }

    if (fread(image_buffer, 1, image_size, image_file) != image_size)
    {
        perror("Error reading image file");
        fclose(image_file);
        free(image_buffer);
        return;
    }

    fclose(image_file);

    // Send HTTP header
    const char *http_header = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\n\r\n";
    //const char *html_body_start = "<!DOCTYPE html>\n<html>\n<head>\n<title>CS410 Webserver</title>\n<style> body { background-color: white; } h1 { font-size: 16pt; color: red; text-align: center; } img { display: block; margin: 0 auto; }</style>\n</head>\n<body>\n<h1>CS410 Webserver</h1>\n<br>\n<img src=\"/home/daniel/Desktop/assignment3/histogram.jpg\">\n</body>\n</html>";

    // Send the HTML content to the client
    send(client_socket, http_header, strlen(http_header), 0);

    // Send image data
    send(client_socket, image_buffer, image_size, 0);

    free(image_buffer);
}

// void histogram(int client_socket, char*address)
// {
//     char tmp[100];
//     sprintf(tmp, "/usr/bin/python3 my-histogram.py %s", address);
//     system(tmp);
//     // Prepare the HTML content with embedded histogram image
//     const char *html_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
//     const char *html_body_start = "<!DOCTYPE html>\n<html>\n<head>\n<title>CS410 Webserver</title>\n<style> body { background-color: white; } h1 { font-size: 16pt; color: red; text-align: center; } img { display: block; margin: 0 auto; }</style>\n</head>\n<body>\n<h1>CS410 Webserver</h1>\n<br>\n<img src=\"data:image/jpeg;base64,";
//     const char *html_body_end = "\"></body>\n</html>";

//     // Send the HTML header
//     send(client_socket, html_header, strlen(html_header), 0);

//     // Read the image file
//     FILE *image_file = fopen("/home/daniel/Desktop/assignment3/histogram.jpg", "rb");
//     if (!image_file)
//     {
//         perror("Error opening image file");
//         return;
//     }

//     // Determine the image file size
//     fseek(image_file, 0, SEEK_END);
//     long image_size = ftell(image_file);
//     rewind(image_file);

//     // Allocate memory for the image buffer
//     char *image_buffer = (char *)malloc(image_size);
//     if (!image_buffer)
//     {
//         perror("Error allocating memory for image buffer");
//         fclose(image_file);
//         return;
//     }

//     // Read image data into the buffer
//     if (fread(image_buffer, 1, image_size, image_file) != image_size)
//     {
//         perror("Error reading image file");
//         fclose(image_file);
//         free(image_buffer);
//         return;
//     }

//     fclose(image_file);

//     // Encode image buffer to base64
//     char *base64_image = base64_encode((const unsigned char *)image_buffer, image_size);

//     // Concatenate HTML content with base64 encoded image
//     char *html_content = (char *)malloc(strlen(html_body_start) + strlen(base64_image) + strlen(html_body_end) + 1);
//     sprintf(html_content, "%s%s%s", html_body_start, base64_image, html_body_end);

//     // Send the HTML content to the client
//     send(client_socket, html_content, strlen(html_content), 0);

//     // Free allocated memory
//     free(image_buffer);
//     free(base64_image);
//     free(html_content);
// }

void servConn(int port)
{
    int sd, new_sd, arduino_sd, client_sd;
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

                if (strstr(requested_resource, "arduino-message") != NULL)
                {
                    arduino_sd = new_sd;
                    if (strstr(requested_resource, "intruder") != NULL)
                    {
                        char response[1024];
                        strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                        strcat(response, "WARNING INTRUDER");
                        send_response(client_sd, response);
                    }
                }
                else
                {
                    client_sd = new_sd;

                    if (strstr(requested_resource, "security") != NULL)
                    {
                        if (strstr(requested_resource, "on") != NULL)
                        {
                            char response[1024];
                            strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                            strcat(response, "turned on security mode");
                            send_response(client_sd, response);

                            char ard_response[1024];
                            strcat(ard_response, "security=on");
                            send_response(arduino_sd, ard_response);
                        }
                        else if (strstr(requested_resource, "off") != NULL)
                        {
                            char response[1024];
                            strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                            strcat(response, "turned off security mode");
                            send_response(client_sd, response);

                            char ard_response[1024];
                            strcat(ard_response, "security=off");
                            send_response(arduino_sd, ard_response);
                        }
                        else
                        {
                            send_404_response(client_sd);
                        }
                    }
                    else if (strstr(requested_resource, ".html") != NULL)
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
                    else if (strstr(requested_resource, "my-histogram") != NULL)
                    {
                        char *eql = strchr(requested_resource, '=');
                        if (eql != NULL)
                        {
                            histogram(new_sd, eql + 1);
                        }
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
