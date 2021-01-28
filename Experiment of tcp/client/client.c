#include "client.h"

int main()
{
    unsigned short port;       /* port client will connect to         */
    char buf[BUF_SIZE];        /* data buffer for sending & receiving */
    struct hostent *hostnm;    /* server host name information        */
    struct sockaddr_in client; /* client address                      */
    struct sockaddr_in server;
    int s; /* client socket                       */

    struct timeval tv_out;
    tv_out.tv_sec = 0;
    tv_out.tv_usec = 100000;

    bzero(buf, BUF_SIZE);
    bzero(&client, sizeof(client));
    /*
     * Put the server information into the server structure.
     * The port must be put into network byte order.
     */
    client.sin_family = AF_INET;
    client.sin_port = htons(MYPORT);
    client.sin_addr.s_addr = inet_addr(SERVER_IP);

    /*
     * Get a stream socket.
     */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket()");
        exit(3);
    }

    // // 绑定客户端的socket和客户端的socket地址结构 非必需
    // if (-1 == (bind(s, (struct sockaddr *)&client, sizeof(client))))
    // {
    //     perror("Bind()");
    //     exit(1);
    // }

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) == 0)
    {
        perror("Server IP Address Error:");
        exit(1);
    }
    server.sin_port = htons(MYPORT);
    socklen_t server_addr_length = sizeof(server);

    printf("Connect %s: %d ...\n", SERVER_IP, MYPORT);
    /*
     * Connect to the server.
     */
    if (connect(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Connect()");
        exit(4);
    }
    printf("Server connected successfully.\n");

    char cmd[FILE_NAME_MAX_LEN];
    bzero(cmd, FILE_NAME_MAX_LEN);
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));
    printf(">> ");
    while (fgets(cmd, sizeof(cmd), stdin) != NULL)
    {
        for (int i = 0; i < sizeof(cmd); i++)
        {
            if (cmd[i] == '\n')
            {
                cmd[i] = '\0';
            }
        }
        strncpy(buf, cmd, strlen(cmd) > BUF_SIZE ? BUF_SIZE : strlen(cmd));
        if (send(s, buf, sizeof(buf), 0) < 0)
        {
            perror("Send()");
            exit(5);
        }
        if (strncmp(cmd, "get", 3) == 0)
        {
            char *fn = cmd + 4;
            FILE *fp = fopen(fn, "w");
            if (NULL == fp)
            {
                printf("File: %s Can Not Open To Write\n", fn);
                exit(1);
            }
            // 从服务器接收数据到buffer中
            // 每接收一段数据，便将其写入文件中，循环直到文件接收完并写完为止
            bzero(buf, BUF_SIZE);
            int length = 0;
            while ((length = recv(s, buf, BUF_SIZE, 0)) > 0)
            {
                if (fwrite(buf, sizeof(char), length, fp) < length)
                {
                    printf("File: %s Write Failed\n", fn);
                    break;
                }
                bzero(buf, BUF_SIZE);
            }
            // 接收成功后，关闭文件，关闭socket
            printf("Receive File: %s Successfully.\n", fn);
            fclose(fp);
        }
        else if (strncmp(cmd, "put", 3) == 0)
        {
            char *fn = cmd + 4;
            // 打开文件并读取文件数据
            FILE *fp = fopen(fn, "r");
            if (NULL == fp)
            {
                printf("File: %s Not Found\n", fn);
            }
            else
            {
                bzero(buf, BUF_SIZE);
                int length = 0;
                // 每读取一段数据，便将其发送给客户端，循环直到文件读完为止
                while ((length = fread(buf, sizeof(char), BUF_SIZE, fp)) > 0)
                {
                    if (send(s, buf, length, 0) < 0)
                    {
                        printf("Send File: %s Failed.\n", fn);
                        exit(7);
                    }
                    bzero(buf, BUF_SIZE);
                }
                // 关闭文件
                fclose(fp);
                printf("Send File: %s Successfully.\n", fn);
            }
        }
        else
        {
            printf("Command Error. Usage: put/get <filename>\n");
            exit(0);
        }
        printf(">> ");
    }
    close(s);
    exit(0);
}