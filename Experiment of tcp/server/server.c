#include "server.h"

int main()
{
    int s;                     // socket for accepting connections
    struct sockaddr_in client; // client address information
    socklen_t namelen;         // length of client name
    int ns;                    // socket connected to client          */

    s = start_server();

    while (1)
    {
        namelen = sizeof(client);
        if ((ns = accept(s, (struct sockaddr *)&client, &namelen)) == -1)
        {
            perror("Accept()");
            continue;
        }
        printf("New Client %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        pthread_t tid;
        pthread_create(&tid, NULL, (void *)(&handle), (void *)(&ns));
        pthread_detach(tid);
    }
    close(s);
    exit(0);
}

static void handle(void *argc)
{
    int ns = *((int *)argc);
    char buf[BUF_SIZE]; // buffer for sending & receiving data
    char file_name[FILE_NAME_MAX_LEN];
    struct timeval tv_out1;
    tv_out1.tv_sec = 0;
    tv_out1.tv_usec = 100000;
    struct timeval tv_out0;
    tv_out0.tv_sec = 0;
    tv_out0.tv_usec = 0;

    while (1)
    {
        bzero(buf, BUF_SIZE);
        bzero(file_name, FILE_NAME_MAX_LEN);
        pthread_t pid = pthread_self();
        memset(buf, 0, sizeof(buf));
        /*
         * Receive the message on the newly connected socket.
         */
        setsockopt(ns, SOL_SOCKET, SO_RCVTIMEO, &tv_out0, sizeof(tv_out0));
        if (recv(ns, buf, sizeof(buf), 0) == -1)
        {
            perror("Recv()");
            exit(6);
        }

        if (strncmp(buf, "get", 3) == 0)
        {
            strncpy(file_name, buf + 4, strlen(buf + 4) > FILE_NAME_MAX_LEN ? FILE_NAME_MAX_LEN : strlen(buf + 4));

            // 打开文件并读取文件数据
            FILE *fp = fopen(file_name, "r");
            if (NULL == fp)
            {
                printf("[pid:%lu] File: %s Not Found\n",pid ,file_name);
            }
            else
            {
                bzero(buf, BUF_SIZE);
                int length = 0;
                // 每读取一段数据，便将其发送给客户端，循环直到文件读完为止
                while ((length = fread(buf, sizeof(char), BUF_SIZE, fp)) > 0)
                {
                    if (send(ns, buf, length, 0) < 0)
                    {
                        printf("[pid:%lu] Send File: %s Failed.\n",pid , file_name);
                        exit(7);
                    }
                    bzero(buf, BUF_SIZE);
                }
                // 关闭文件
                fclose(fp);
                printf("[pid:%lu] Send File: %s Successfully.\n", pid, file_name);
            }
        }
        else if (strncmp(buf, "put", 3) == 0)
        {
            setsockopt(ns, SOL_SOCKET, SO_RCVTIMEO, &tv_out0, sizeof(tv_out0));
            strncpy(file_name, buf + 4, strlen(buf + 4) > FILE_NAME_MAX_LEN ? FILE_NAME_MAX_LEN : strlen(buf + 4));
            
            FILE *fp = fopen(file_name, "w");
            if (NULL == fp)
            {
                printf("[pid:%lu] File: %s Can Not Open To Write.\n", pid , file_name);
                exit(1);
            }
            // 从客户端接收数据到buffer中
            // 每接收一段数据，便将其写入文件中，循环直到文件接收完并写完为止
            bzero(buf, BUF_SIZE);
            int length = 0;
            setsockopt(ns, SOL_SOCKET, SO_RCVTIMEO, &tv_out1, sizeof(tv_out1));
            while ((length = recv(ns, buf, BUF_SIZE, 0)) > 0)
            {
                if (fwrite(buf, sizeof(char), length, fp) < length)
                {
                    printf("File: %s Write Failed.\n", file_name);
                    break;
                }
                bzero(buf, BUF_SIZE);
            }
            // 接收成功后，关闭文件，关闭socket
            printf("[pid:%lu] Receive File: %s Successfully.\n", pid, file_name);
            fclose(fp);
        }
        else
        {
            printf("Command From Client Error.\n");
            continue;
        }
    }
    pthread_exit(NULL);
    close(ns);
}

int start_server()
{
    int s;
    struct sockaddr_in server; // server address information
    bzero(&server, sizeof(server));
    /*
     * Bind the socket to the server address.
     */
    server.sin_family = AF_INET;
    server.sin_port = htons(MYPORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /*
     * Get a socket for accepting connections.
     */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket()");
        exit(2);
    }
    //AF_INET: TCP/IP – IPv4
    //SOCK_STREAM: 流服务，TCP协议    (SOCK_DGRAM:数据报服务，UDP协议)
    //0: 由系统根据服务类型选择默认的协议
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(s, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("Bind()");
        exit(3);
    }
    //s: 套接字描述符
    //(struct sockaddr *)&server: 本地地址，IP地址和端口号
    //sizeof(server): 地址长度

    /*
     * Listen for connections. Specify the backlog as 1.
     */
    printf("port: %d\n", MYPORT);
    if (listen(s, QUEUE) != 0)
    {
        perror("Listen()");
        exit(4);
    }
    return s;
}