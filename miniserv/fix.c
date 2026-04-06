#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

typedef struct s_client {
    int id;
    char msg[100000];
} t_client;

t_client client[2048];
fd_set read_fds, write_fds, all_fds;
int maxfd = 0, next_id = 0;
char s_buf[120000], r_buf[110000];

void err(char *msg) {
    if (msg) 
        write(2, msg, strlen(msg));
    else 
        write(2, "Fatal error", 11);
    write(2, "\n", 1);
    exit(1);
}

void send_all(int except) {
    for (int fd = 0; fd <= maxfd; fd++) {
        if (FD_ISSET(fd, &write_fds) && fd != except)
            send(fd, s_buf, strlen(s_buf), 0);
    }
}

int main(int ac, char **av) {
    if (ac != 2) 
        err("Wrong number of arguments");

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) 
        err(NULL);
    maxfd = sfd;

    FD_ZERO(&all_fds);
    FD_SET(sfd, &all_fds);

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    serv_addr.sin_port = htons(atoi(av[1]));

    if(bind(sfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1 \
        || listen(sfd, 128) == -1)
        err(NULL); 

    while (1) {
        read_fds = write_fds = all_fds;
        if (select(maxfd + 1, &read_fds, &write_fds, 0, 0) == -1) 
            continue;
            
        for (int fd = 0; fd <= maxfd; fd++) {
            if (!FD_ISSET(fd, &read_fds)) 
                continue;
            if (fd == sfd) {
                int cfd = accept(sfd, NULL, NULL);                
                if (cfd == -1) 
                    continue;
                if (cfd > maxfd) 
                    maxfd = cfd;
                client[cfd].id = next_id++;
                client[cfd].msg[0] = '\0';
                FD_SET(cfd, &all_fds);
                sprintf(s_buf, "server: client %d just arrived\n", client[cfd].id);
                send_all(cfd);
                break;
            } else {
                int n = recv(fd, r_buf, 100000, 0);
                if (n <= 0) {
                    sprintf(s_buf, "server: client %d just left\n", client[fd].id);
                    send_all(fd);
                    FD_CLR(fd, &all_fds);
                    close(fd);
                    break;
                }
                r_buf[n] = '\0';
                for (int i = 0; i < n; i++) {
                    int l = strlen(client[fd].msg);
                    client[fd].msg[l] = r_buf[i];
                    client[fd].msg[l+1] = '\0';
                    if (client[fd].msg[l] == '\n') {
                        sprintf(s_buf, "client %d: %s", client[fd].id, client[fd].msg);
                        send_all(fd);
                        client[fd].msg[0] = '\0';
                    }
                }
            }
        }
    }
}