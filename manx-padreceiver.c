#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define RXBUFFSIZE 1024
#define RXPORT 7000
#define OUTFILE "/tmp/padfile"

void printBuff(int n, char *buff)
{
    time_t ticks;
    char timeStr[80];
    struct tm *tmPtr;

    ticks = time(NULL);
    tmPtr = localtime(&ticks);
    strftime(timeStr, 80, "%d/%m/%y %T", tmPtr);

    printf("[%s] ", timeStr);

    while(n)
    {
        if(isprint(*buff))
        {
            putchar(*buff);
        }
        buff++;
        n--;
    }

    printf("\n");
}

void writeFile(int n, char *buff, char *fname)
{
    int fd;

    fd = open(fname, O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH );
    if(fd < 0)
    {
        perror("open");
        return;
    }

    while(n)
    {
        if(isprint(*buff))
        {
            write(fd, buff, 1);
        }

        buff++;
        n--;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    int listenfd; 
    int connfd;
    struct sockaddr_in serv_addr;
    struct sockaddr_in client_addr;
    socklen_t sock_size;
    struct in_addr allowed_client;
    int any_client;
    char *rxBuff;
    int n;
    int rxPort;
    int buffSize;
    char outfile[256];
    int opt;

    setvbuf(stdout, NULL, _IOLBF, 0);

    rxPort = RXPORT;
    buffSize = RXBUFFSIZE;
    sprintf(outfile, "OUTFILE");
    any_client = 0;

    do
    {
        opt = getopt(argc, argv, "o:b:p:i:");
        if(opt != -1)
        {
            switch(opt)
            {
                case 'b':
                    buffSize = atoi(optarg);
                    break;

                case 'p':
                    rxPort = atoi(optarg);
                    break;

                case 'i':
                    inet_aton(optarg, &allowed_client);
                    any_client = (int)~0;
                    break;

                case 'o':
                    strcpy(outfile, optarg);
                    break;

                default:
                    printf("manx-padreceiver [-i client IP] [-p port] [-b buffer size] [-o output file]\n");
                    return -1;
            }
        }
    }
    while(opt != -1);

    rxBuff = (char *)malloc(buffSize);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    printf("\n\n");
    printf("Manx Radio PAD receiver, compiled on %s, %s\n", __DATE__, __TIME__);
    printf("  Allowing connections from ");
    if(any_client == 0)
    {
        printf("ANYONE\n");
    }
    else
    {
        printf("%s\n", inet_ntoa(allowed_client));
    }
    printf("  Listening on port %d\n", rxPort);
    printf("  Receive buffer size %d\n", buffSize);
    printf("  Writing to %s\n", outfile);
    printf("\n");
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(rxPort);

    printf("  Binding . ");
    fflush(stdout);
    while(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        sleep(5);
        printf(". ");
        fflush(stdout);
    }

    if(listen(listenfd, 10) < 0)
    {
        perror("listen");
        return -1;
    }

    printf("listening\n\n");

    while(1)
    {
        connfd = accept(listenfd, (struct sockaddr*)&client_addr, &sock_size);
//        connfd = accept(listenfd, NULL, NULL);

        if(any_client != 0 &&
                          client_addr.sin_addr.s_addr != allowed_client.s_addr)
        {

            sprintf(rxBuff, "Ignoring connection from %s/%d\n",
                                 inet_ntoa(client_addr.sin_addr),
                                 (int)ntohs(client_addr.sin_port));
            printBuff(strlen(rxBuff), rxBuff);

        }
        else
        {
      
            sprintf(rxBuff, "Connection from %s/%d\n",
                                 inet_ntoa(client_addr.sin_addr),
                                 (int)ntohs(client_addr.sin_port));
            printBuff(strlen(rxBuff), rxBuff);

            do
            {
                n = read(connfd, rxBuff, RXBUFFSIZE - 1);
                if(n)
                {
                    printBuff(n, rxBuff);
                    writeFile(n, rxBuff, outfile);
                }
            }
            while(n > 0);

        }

        close(connfd);
        sleep(1);
    }
}

