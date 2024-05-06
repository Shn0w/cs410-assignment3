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
    exit(-1);
  }

  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&sock_opt_val,
                 sizeof(sock_opt_val)) < 0)
  {
    perror("(servConn): Failed to set SO_REUSEADDR on INET socket");
    exit(-1);
  }

  name.sin_family = AF_INET;
  name.sin_port = htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sd, (struct sockaddr *)&name, sizeof(name)) < 0)
  {
    perror("(servConn): bind() error");
    exit(-1);
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
      exit(-1);
    }

    if (fork() == 0)
    { /* Child process. */
      close(sd);
      memset(data, 0, sizeof(data));
      // read (new_sd, &data, 25); /* Read our string: "Hello, World!" */
      recv(new_sd, data, sizeof(data), 0); /* Read request from client */
      printf("%s\n", data);

      // Parse HTTP request to extract requested resource
      char *http_method[16];         // Buffer to store the HTTP method (e.g., "GET")
      char *requested_resource[256]; // Buffer to store the requested directory

      // Parse the request string
      sscanf(data, "%s %s", http_method, requested_resource);

      // Print the extracted directory
      printf("Extracted directory: %s\n", requested_resource);
      printf("method: %s\n", http_method);
      // char *requested_resource = strtok(data, " ");
      // char *http_method = strtok(NULL, " \t");

      if (strcmp(http_method, "GET") == 0)
      {
        printf("debug: in get\n");
        // Check if the requested resource is a directory
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir(requested_resource)) != NULL)
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
      //   else
      //   {
      //     // Check if the requested resource exists
      //     FILE *file = fopen(requested_resource, "r");
      //     if (file != NULL)
      //     {
      //       // Send file contents
      //       char response[1024];
      //       strcpy(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      //       char buffer[1024];
      //       while (fgets(buffer, sizeof(buffer), file) != NULL)
      //       {
      //         strcat(response, buffer);
      //       }
      //       fclose(file);
      //       send_response(new_sd, response);
      //     }
      //     else
      //     {
      //       send_404_response(new_sd);
      //     }
      //   }
      }
      else
      {
        // Unsupported HTTP method
        const char *response = "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n";
        send_response(new_sd, response);
      }
      close(new_sd);
      exit(EXIT_SUCCESS);
    }
    else
    {
      close(new_sd);
    }
  }

  // for (;;)
  // {
  //   cli_len = sizeof(cli_name);
  //   new_sd = accept(sd, (struct sockaddr *)&cli_name, &cli_len);
  //   printf("Assigning new socket descriptor:  %d\n", new_sd);

  //   if (new_sd < 0)
  //   {
  //     perror("(servConn): accept() error");
  //     exit(-1);
  //   }

  //   if (fork() == 0)
  //   { /* Child process. */
  //     close(sd);
  //     read(new_sd, &data, 25); /* Read our string: "Hello, World!" */
  //     printf("Received string = %s\n", data);
  //     exit(0);
  //   }
  // }
}

int main(int argc, char *argv[])
{
  if (argv[1] == NULL)
  {
    perror("incorrect input");
  }
  servConn(atoi(argv[1])); /* Server port. */

  return 0;
}
