#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 8192
#define WEBROOT "./public"

// 全局变量定义
int server_fd;

// 获取文件的MIME类型
const char *get_mime_type(const char *file_path)
{
    char *ext = strrchr(file_path, '.');
    if (ext == NULL)
        return "application/octet-stream";

    ext++; // 跳过点号
    for (char *p = ext; *p; p++)
    {
        *p = tolower(*p);
    }

    if (strcmp(ext, "html") == 0)
        return "text/html";
    if (strcmp(ext, "css") == 0)
        return "text/css";
    if (strcmp(ext, "js") == 0)
        return "application/javascript";
    if (strcmp(ext, "json") == 0)
        return "application/json";
    if (strcmp(ext, "png") == 0)
        return "image/png";

    return "text/plain";
}

// 发送静态文件
void send_file(int client_fd, const char *file_path)
{
    // 打开文件
    int fd = open(file_path, O_RDONLY);
    if (fd == -1)
    {
        // 文件不存在
        char response[512];
        sprintf(response,
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "\r\n"
                "<html><body><h1>404 Not Found</h1></body></html>");
        write(client_fd, response, strlen(response));
        return;
    }

    // 获取文件大小
    struct stat file_stat;
    fstat(fd, &file_stat);

    // 发送HTTP头
    char header[512];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "\r\n",
            get_mime_type(file_path), file_stat.st_size);
    write(client_fd, header, strlen(header));

    // 发送文件内容
    char buffer[4096];
    int bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        write(client_fd, buffer, bytes_read);
    }

    close(fd);
}

// 发送JSON响应
void send_json(int client_fd, const char *json_data)
{
    char header[512];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %ld\r\n"
            "\r\n",
            strlen(json_data));
    write(client_fd, header, strlen(header));
    write(client_fd, json_data, strlen(json_data));
}

// 处理HTTP请求
void handle_request(int client_fd, char *request)
{
    // 解析请求行
    char method[16], path[256], version[16];
    sscanf(request, "%s %s %s", method, path, version);

    printf("收到请求: %s %s\n", method, path);

    // 处理API请求
    if (strncmp(path, "/api/", 5) == 0)
    {
        handle_api_request(client_fd, method, path, request);
        return;
    }

    // 处理静态文件请求
    if (strcmp(path, "/") == 0)
    {
        strcpy(path, "/index.html");
    }

    char file_path[512];
    sprintf(file_path, "%s%s", WEBROOT, path);

    // 对于SPA应用，如果文件不存在，返回index.html
    struct stat st;
    if (stat(file_path, &st) == -1)
    {
        sprintf(file_path, "%s/index.html", WEBROOT);
    }

    send_file(client_fd, file_path);
}

// 主函数
int main()
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 创建套接字
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 设置端口重用
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 设置服务器地址
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定地址
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, 10) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("服务器已启动，监听端口 %d\n", PORT);

    while (1)
    {
        int client_fd;
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }

        // 读取请求
        char buffer[BUFFER_SIZE] = {0};
        read(client_fd, buffer, BUFFER_SIZE);

        // 处理请求
        handle_request(client_fd, buffer);

        // 关闭连接
        close(client_fd);
    }

    close(server_fd);
    return 0;
}