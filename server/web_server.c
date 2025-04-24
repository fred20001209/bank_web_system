#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h> // 为isxdigit函数
#include <errno.h> // 添加这一行，用于ENOMSG的定义

#define PORT 8080
#define WEB_ROOT "/home/bank_web_system/web"

// 消息队列相关定义
static int g_reqid = -1; // 请求消息队列ID
static int g_resid = -1; // 响应消息队列ID

// 消息类型常量 - 与原系统保持一致
#define TYPE_OPEN 11
#define TYPE_CLOSE 12
#define TYPE_SAVE 13
#define TYPE_WITHDRAW 14
#define TYPE_QUERY 15
#define TYPE_TRANSFER 16

// 银行账户结构体
typedef struct tag_Account
{
    int id;
    char name[256];
    char passwd[6];
    double balance;
} ACCOUNT;

// 开户请求结构体
typedef struct tag_OpenRequest
{
    long type;
    pid_t pid;
    char name[256];
    char passwd[6];
    double balance;
} OPEN_REQUEST;

// 开户响应结构体
typedef struct tag_OpenRespond
{
    long type;
    char error[512];
    int id;
} OPEN_RESPOND;

// 销户请求结构体
typedef struct tag_CloseRequest
{
    long type;
    pid_t pid;
    int id;
    char name[256];
    char passwd[6];
} CLOSE_REQUEST;

// 销户响应结构体
typedef struct tag_CloseRespond
{
    long type;
    char error[512];
    double balance;
} CLOSE_RESPOND;

// 存款请求结构体
typedef struct tag_SaveRequest
{
    long type;
    pid_t pid;
    int id;
    char name[256];
    double money;
} SAVE_REQUEST;

// 存款响应结构体
typedef struct tag_SaveRespond
{
    long type;
    char error[512];
    double balance;
} SAVE_RESPOND;

// 取款请求结构体
typedef struct tag_WithdrawRequest
{
    long type;
    pid_t pid;
    int id;
    char name[256];
    char passwd[6]; // 修改为6字节，与账户结构体一致
    double money;
} WITHDRAW_REQUEST;

// 取款响应结构体
typedef struct tag_WithdrawRespond
{
    long type;
    char error[512];
    double balance;
} WITHDRAW_RESPOND;

// 查询请求结构体
typedef struct tag_QueryRequest
{
    long type;
    pid_t pid;
    int id;
    char name[256];
    char passwd[6]; // 修改为6字节
} QUERY_REQUEST;

// 查询响应结构体
typedef struct tag_QueryRespond
{
    long type;
    char error[512];
    double balance;
} QUERY_RESPOND;

// 转账请求结构体
typedef struct tag_TransferRequest
{
    long type;
    pid_t pid;
    int id[2];
    char name[2][256];
    char passwd[6]; // 改为6字节
    double money;
} TRANSFER_REQUEST;

// 转账响应结构体
typedef struct tag_TransferRespond
{
    long type;
    char error[512];
    double balance;
} TRANSFER_RESPOND;

// URL解码函数
void url_decode(const char *src, char *dst)
{
    char a, b;
    while (*src)
    {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';

            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';

            *dst++ = 16 * a + b;
            src += 3;
        }
        else if (*src == '+')
        {
            *dst++ = ' ';
            src++;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// 初始化消息队列
int init_msg_queue()
{
    key_t KEY_REQUEST = ftok("/home", 2);
    key_t KEY_RESPOND = ftok("/home", 3);

    printf("尝试连接消息队列，请求键: %d, 响应键: %d\n", KEY_REQUEST, KEY_RESPOND);

    // 尝试连接到现有的消息队列
    if ((g_reqid = msgget(KEY_REQUEST, 0600)) == -1)
    {
        perror("连接请求消息队列失败");
        return -1;
    }

    printf("成功连接请求消息队列，ID: %d\n", g_reqid);

    if ((g_resid = msgget(KEY_RESPOND, 0600)) == -1)
    {
        perror("连接响应消息队列失败");
        return -1;
    }

    printf("成功连接响应消息队列，ID: %d\n", g_resid);
    return 0;
}

// 处理银行业务请求
char *handle_bank_operation(int operation, char *params)
{
    printf("处理银行操作: %d, 参数: %s\n", operation, params);

    // 如果消息队列未初始化，尝试初始化
    if (g_reqid == -1 || g_resid == -1)
    {
        printf("消息队列未初始化，尝试初始化...\n");
        if (init_msg_queue() == -1)
        {
            printf("初始化消息队列失败\n");
            return "{\"result\":1, \"message\":\"无法连接到银行服务器\", \"balance\":0.00}";
        }
    }

    // 解析URL参数到临时变量
    char param_copy[1024];
    strcpy(param_copy, params);

    char name[256] = "";
    char passwd[6] = "";
    int account_id = 0;
    int to_account = 0;
    double amount = 0.0;
    char to_name[256] = ""; // 添加对方姓名变量

    char *token = strtok(param_copy, "&");
    while (token != NULL)
    {
        char key[32], value[256];
        char *eq = strchr(token, '=');
        if (eq)
        {
            strncpy(key, token, eq - token);
            key[eq - token] = '\0';
            strcpy(value, eq + 1);

            // URL解码value
            char decoded_value[256];
            url_decode(value, decoded_value);

            if (strcmp(key, "name") == 0)
            {
                strncpy(name, decoded_value, sizeof(name) - 1);
                printf("参数: name = %s\n", name);
            }
            else if (strcmp(key, "passwd") == 0)
            {
                strncpy(passwd, decoded_value, sizeof(passwd) - 1);
                printf("参数: passwd = %s\n", passwd);
            }
            else if (strcmp(key, "account_id") == 0)
            {
                account_id = atoi(decoded_value);
                printf("参数: account_id = %d\n", account_id);
            }
            else if (strcmp(key, "amount") == 0)
            {
                amount = atof(decoded_value);
                printf("参数: amount = %.2f\n", amount);
            }
            else if (strcmp(key, "to_account") == 0)
            {
                to_account = atoi(decoded_value);
                printf("参数: to_account = %d\n", to_account);
            }
            else if (strcmp(key, "to_name") == 0) // 添加对方姓名解析
            {
                strncpy(to_name, decoded_value, sizeof(to_name) - 1);
                printf("参数: to_name = %s\n", to_name);
            }
        }

        token = strtok(NULL, "&");
    }

    static char result[1024]; // 返回结果
    int ret = -1;             // 函数返回值

    // 根据操作类型使用对应的消息结构体
    switch (operation)
    {
    case 1: // 开户
    {
        OPEN_REQUEST req;
        memset(&req, 0, sizeof(req));
        req.type = TYPE_OPEN;
        req.pid = getpid();
        strncpy(req.name, name, sizeof(req.name) - 1);
        strncpy(req.passwd, passwd, sizeof(req.passwd) - 1);
        req.balance = amount;

        printf("发送开户请求到队列 ID: %d\n", g_reqid);
        if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(long), 0) == -1)
        {
            perror("发送请求失败");
            return "{\"result\":1, \"message\":\"发送请求消息失败\", \"balance\":0.00}";
        }

        // 接收响应消息
        printf("等待开户响应消息...\n");
        OPEN_RESPOND res;
        memset(&res, 0, sizeof(res));

        int retry_count = 0;
        while (retry_count < 10)
        {
            ret = msgrcv(g_resid, &res, sizeof(res) - sizeof(long), req.pid, IPC_NOWAIT);
            if (ret != -1)
            {
                break; // 接收成功
            }

            if (errno != ENOMSG)
            {
                perror("接收响应失败");
                return "{\"result\":1, \"message\":\"接收响应消息失败\", \"balance\":0.00}";
            }

            printf("等待开户响应，重试 %d/10...\n", retry_count + 1);
            retry_count++;
            sleep(1); // 等待1秒后重试
        }

        if (ret == -1)
        {
            printf("等待开户响应超时\n");
            return "{\"result\":1, \"message\":\"等待银行系统响应超时，请稍后再试\", \"balance\":0.00}";
        }

        printf("收到开户响应: 账户ID=%d, 错误信息=%s\n", res.id, res.error);

        // 构建JSON响应
        if (strlen(res.error) > 0)
        {
            sprintf(result, "{\"result\":1, \"message\":\"%s\", \"balance\":0.00}", res.error);
        }
        else
        {
            sprintf(result, "{\"result\":0, \"message\":\"%d\", \"balance\":%.2f}",
                    res.id, req.balance);
        }
        break;
    }

    case 2: // 销户
    {
        CLOSE_REQUEST req;
        memset(&req, 0, sizeof(req));
        req.type = TYPE_CLOSE;
        req.pid = getpid();
        req.id = account_id;
        strncpy(req.name, name, sizeof(req.name) - 1); // 添加名称字段
        strncpy(req.passwd, passwd, sizeof(req.passwd) - 1);

        printf("发送销户请求到队列 ID: %d\n", g_reqid);
        if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(long), 0) == -1)
        {
            perror("发送请求失败");
            return "{\"result\":1, \"message\":\"发送请求消息失败\", \"balance\":0.00}";
        }

        // 接收响应消息
        printf("等待销户响应消息...\n");
        CLOSE_RESPOND res;
        memset(&res, 0, sizeof(res));

        int retry_count = 0;
        while (retry_count < 10)
        {
            ret = msgrcv(g_resid, &res, sizeof(res) - sizeof(long), req.pid, IPC_NOWAIT);
            if (ret != -1)
            {
                break; // 接收成功
            }

            if (errno != ENOMSG)
            {
                perror("接收响应失败");
                return "{\"result\":1, \"message\":\"接收响应消息失败\", \"balance\":0.00}";
            }

            printf("等待销户响应，重试 %d/10...\n", retry_count + 1);
            retry_count++;
            sleep(1); // 等待1秒后重试
        }

        if (ret == -1)
        {
            printf("等待销户响应超时\n");
            return "{\"result\":1, \"message\":\"等待银行系统响应超时，请稍后再试\", \"balance\":0.00}";
        }

        printf("收到销户响应: 余额=%.2f, 错误信息=%s\n", res.balance, res.error);

        // 构建JSON响应
        if (strlen(res.error) > 0)
        {
            sprintf(result, "{\"result\":1, \"message\":\"%s\", \"balance\":0.00}", res.error);
        }
        else
        {
            sprintf(result, "{\"result\":0, \"message\":\"账户已成功注销\", \"balance\":%.2f}", res.balance);
        }
        break;
    }

    case 3: // 存款
    {
        SAVE_REQUEST req;
        memset(&req, 0, sizeof(req));
        req.type = TYPE_SAVE;
        req.pid = getpid();
        req.id = account_id;
        strncpy(req.name, name, sizeof(req.name) - 1);
        req.money = amount;

        printf("发送存款请求到队列 ID: %d\n", g_reqid);
        if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(long), 0) == -1)
        {
            perror("发送请求失败");
            return "{\"result\":1, \"message\":\"发送请求消息失败\", \"balance\":0.00}";
        }

        // 接收响应消息
        printf("等待存款响应消息...\n");
        SAVE_RESPOND res;
        memset(&res, 0, sizeof(res));

        int retry_count = 0;
        while (retry_count < 10)
        {
            ret = msgrcv(g_resid, &res, sizeof(res) - sizeof(long), req.pid, IPC_NOWAIT);
            if (ret != -1)
            {
                break; // 接收成功
            }

            if (errno != ENOMSG)
            {
                perror("接收响应失败");
                return "{\"result\":1, \"message\":\"接收响应消息失败\", \"balance\":0.00}";
            }

            printf("等待存款响应，重试 %d/10...\n", retry_count + 1);
            retry_count++;
            sleep(1); // 等待1秒后重试
        }

        if (ret == -1)
        {
            printf("等待存款响应超时\n");
            return "{\"result\":1, \"message\":\"等待银行系统响应超时，请稍后再试\", \"balance\":0.00}";
        }

        printf("收到存款响应: 余额=%.2f, 错误信息=%s\n", res.balance, res.error);

        // 构建JSON响应
        if (strlen(res.error) > 0)
        {
            sprintf(result, "{\"result\":1, \"message\":\"%s\", \"balance\":0.00}", res.error);
        }
        else
        {
            sprintf(result, "{\"result\":0, \"message\":\"存款成功\", \"balance\":%.2f}", res.balance);
        }
        break;
    }

    case 4: // 取款
    {
        WITHDRAW_REQUEST req;
        memset(&req, 0, sizeof(req));
        req.type = TYPE_WITHDRAW;
        req.pid = getpid();
        req.id = account_id;
        strncpy(req.name, name, sizeof(req.name) - 1); // 添加名称字段
        strncpy(req.passwd, passwd, sizeof(req.passwd) - 1);
        req.money = amount;

        printf("发送取款请求到队列 ID: %d\n", g_reqid);
        if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(long), 0) == -1)
        {
            perror("发送请求失败");
            return "{\"result\":1, \"message\":\"发送请求消息失败\", \"balance\":0.00}";
        }

        // 接收响应消息
        printf("等待取款响应消息...\n");
        WITHDRAW_RESPOND res;
        memset(&res, 0, sizeof(res));

        int retry_count = 0;
        while (retry_count < 10)
        {
            ret = msgrcv(g_resid, &res, sizeof(res) - sizeof(long), req.pid, IPC_NOWAIT);
            if (ret != -1)
            {
                break; // 接收成功
            }

            if (errno != ENOMSG)
            {
                perror("接收响应失败");
                return "{\"result\":1, \"message\":\"接收响应消息失败\", \"balance\":0.00}";
            }

            printf("等待取款响应，重试 %d/10...\n", retry_count + 1);
            retry_count++;
            sleep(1); // 等待1秒后重试
        }

        if (ret == -1)
        {
            printf("等待取款响应超时\n");
            return "{\"result\":1, \"message\":\"等待银行系统响应超时，请稍后再试\", \"balance\":0.00}";
        }

        printf("收到取款响应: 余额=%.2f, 错误信息=%s\n", res.balance, res.error);

        // 构建JSON响应
        if (strlen(res.error) > 0)
        {
            sprintf(result, "{\"result\":1, \"message\":\"%s\", \"balance\":0.00}", res.error);
        }
        else
        {
            sprintf(result, "{\"result\":0, \"message\":\"取款成功\", \"balance\":%.2f}", res.balance);
        }
        break;
    }

    case 5: // 查询
    {
        QUERY_REQUEST req;
        memset(&req, 0, sizeof(req));
        req.type = TYPE_QUERY;
        req.pid = getpid();
        req.id = account_id;
        strncpy(req.name, name, sizeof(req.name) - 1); // 添加名称字段
        strncpy(req.passwd, passwd, sizeof(req.passwd) - 1);

        printf("发送查询请求到队列 ID: %d\n", g_reqid);
        if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(long), 0) == -1)
        {
            perror("发送请求失败");
            return "{\"result\":1, \"message\":\"发送请求消息失败\", \"balance\":0.00}";
        }

        // 接收响应消息
        printf("等待查询响应消息...\n");
        QUERY_RESPOND res;
        memset(&res, 0, sizeof(res));

        int retry_count = 0;
        while (retry_count < 10)
        {
            ret = msgrcv(g_resid, &res, sizeof(res) - sizeof(long), req.pid, IPC_NOWAIT);
            if (ret != -1)
            {
                break; // 接收成功
            }

            if (errno != ENOMSG)
            {
                perror("接收响应失败");
                return "{\"result\":1, \"message\":\"接收响应消息失败\", \"balance\":0.00}";
            }

            printf("等待查询响应，重试 %d/10...\n", retry_count + 1);
            retry_count++;
            sleep(1); // 等待1秒后重试
        }

        if (ret == -1)
        {
            printf("等待查询响应超时\n");
            return "{\"result\":1, \"message\":\"等待银行系统响应超时，请稍后再试\", \"balance\":0.00}";
        }

        printf("收到查询响应: 余额=%.2f, 错误信息=%s\n", res.balance, res.error);

        // 构建JSON响应
        if (strlen(res.error) > 0)
        {
            sprintf(result, "{\"result\":1, \"message\":\"%s\", \"balance\":0.00}", res.error);
        }
        else
        {
            sprintf(result, "{\"result\":0, \"message\":\"查询成功\", \"balance\":%.2f}", res.balance);
        }
        break;
    }

    case 6: // 转账
    {
        TRANSFER_REQUEST req;
        memset(&req, 0, sizeof(req));
        req.type = TYPE_TRANSFER;
        req.pid = getpid();

        // 使用id数组存储账号
        req.id[0] = account_id; // 转出账户
        req.id[1] = to_account; // 转入账户

        // 使用name数组存储账户名
        strncpy(req.name[0], name, sizeof(req.name[0]) - 1);    // 转出账户名
        strncpy(req.name[1], to_name, sizeof(req.name[1]) - 1); // 转入账户名

        // 存储密码
        strncpy(req.passwd, passwd, sizeof(req.passwd) - 1);

        // 存储转账金额
        req.money = amount;

        printf("发送转账请求到队列 ID: %d\n", g_reqid);
        if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(long), 0) == -1)
        {
            perror("发送请求失败");
            return "{\"result\":1, \"message\":\"发送请求消息失败\", \"balance\":0.00}";
        }

        // 接收响应消息
        printf("等待转账响应消息...\n");
        TRANSFER_RESPOND res;
        memset(&res, 0, sizeof(res));

        int retry_count = 0;
        while (retry_count < 10)
        {
            ret = msgrcv(g_resid, &res, sizeof(res) - sizeof(long), req.pid, IPC_NOWAIT);
            if (ret != -1)
            {
                break; // 接收成功
            }

            if (errno != ENOMSG)
            {
                perror("接收响应失败");
                return "{\"result\":1, \"message\":\"接收响应消息失败\", \"balance\":0.00}";
            }

            printf("等待转账响应，重试 %d/10...\n", retry_count + 1);
            retry_count++;
            sleep(1); // 等待1秒后重试
        }

        if (ret == -1)
        {
            printf("等待转账响应超时\n");
            return "{\"result\":1, \"message\":\"等待银行系统响应超时，请稍后再试\", \"balance\":0.00}";
        }

        printf("收到转账响应: 余额=%.2f, 错误信息=%s\n", res.balance, res.error);

        // 构建JSON响应
        if (strlen(res.error) > 0)
        {
            sprintf(result, "{\"result\":1, \"message\":\"%s\", \"balance\":0.00}", res.error);
        }
        else
        {
            sprintf(result, "{\"result\":0, \"message\":\"转账成功\", \"balance\":%.2f}", res.balance);
        }
        break;
    }

    default:
        sprintf(result, "{\"result\":1, \"message\":\"未知的操作类型\", \"balance\":0.00}");
    }

    return result;
}

// 处理HTTP请求
void handle_http_request(int client_socket)
{
    char buffer[4096];
    int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';

    printf("\n=== 收到新HTTP请求 ===\n");
    printf("请求内容:\n%s\n", buffer);

    // 解析HTTP请求
    char method[10], path[1024], protocol[10];
    sscanf(buffer, "%s %s %s", method, path, protocol);

    printf("解析结果: 方法=%s, 路径=%s, 协议=%s\n", method, path, protocol);

    // 响应头和响应内容
    char response[8192];
    char *content;

    // API请求: /api/operation?params
    if (strncmp(path, "/api/", 5) == 0)
    {
        printf("处理API请求: %s\n", path);
        // 解析操作类型和参数
        char *operation_str = path + 5;
        char *params = strchr(operation_str, '?');
        printf("原始操作字符串: %s\n", operation_str);
        int operation = 0;

        if (params != NULL)
        {
            *params = '\0'; // 分割路径
            params++;       // 参数开始位置
            printf("分离后的操作: %s, 参数: %s\n", operation_str, params);
        }
        else
        {
            params = ""; // 没有参数
            printf("没有参数\n");
        }

        // 映射操作名称到操作ID
        if (strcmp(operation_str, "open") == 0)
            operation = 1;
        else if (strcmp(operation_str, "close") == 0)
            operation = 2;
        else if (strcmp(operation_str, "save") == 0)
            operation = 3;
        else if (strcmp(operation_str, "withdraw") == 0)
            operation = 4;
        else if (strcmp(operation_str, "query") == 0)
            operation = 5;
        else if (strcmp(operation_str, "transfer") == 0)
            operation = 6;

        // 处理银行操作并获取结果
        content = handle_bank_operation(operation, params);

        // 发送JSON响应
        sprintf(response,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: %zu\r\n"
                "Access-Control-Allow-Origin: *\r\n"
                "\r\n%s",
                strlen(content), content);
        printf("发送API响应: %s\n", response);
        write(client_socket, response, strlen(response));
    }
    else
    {
        // 静态文件请求
        char full_path[1024];
        sprintf(full_path, "%s%s", WEB_ROOT, strcmp(path, "/") == 0 ? "/index.html" : path);

        printf("请求静态文件: %s\n", full_path);
        FILE *file = fopen(full_path, "r");
        if (file)
        {
            // 计算文件大小
            fseek(file, 0, SEEK_END);
            long content_length = ftell(file);
            rewind(file);

            // 读取文件内容
            char *file_content = malloc(content_length + 1);
            fread(file_content, 1, content_length, file);
            file_content[content_length] = '\0';

            // 确定内容类型
            const char *content_type = "text/html";
            if (strstr(path, ".css"))
                content_type = "text/css";
            else if (strstr(path, ".js"))
                content_type = "application/javascript";
            else if (strstr(path, ".jpg") || strstr(path, ".jpeg"))
                content_type = "image/jpeg";
            else if (strstr(path, ".png"))
                content_type = "image/png";

            // 发送响应头
            sprintf(response,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: %s\r\n"
                    "Content-Length: %ld\r\n"
                    "\r\n",
                    content_type, content_length);

            printf("发送文件响应头\n");
            write(client_socket, response, strlen(response));

            // 发送文件内容
            printf("发送文件内容，大小: %ld 字节\n", content_length);
            write(client_socket, file_content, content_length);

            free(file_content);
            fclose(file);
            return;
        }
        else
        {
            printf("文件未找到: %s\n", full_path);
            // 文件未找到
            content = "404 Not Found";
            sprintf(response,
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %zu\r\n"
                    "\r\n%s",
                    strlen(content), content);

            write(client_socket, response, strlen(response));
        }
    }
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 初始化连接到银行系统的消息队列
    if (init_msg_queue() == -1)
    {
        printf("警告: 未能连接到银行系统，请确保银行系统服务器已启动\n");
    }
    else
    {
        printf("已成功连接到银行系统\n");
    }

    // 创建Socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Socket创建失败");
        exit(EXIT_FAILURE);
    }

    // 允许重用地址
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 绑定地址和端口
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("绑定Socket失败");
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_socket, 10) < 0)
    {
        perror("监听Socket失败");
        exit(EXIT_FAILURE);
    }

    printf("Web服务器已启动，监听端口 %d\n", PORT);
    printf("可以通过浏览器访问 http://localhost:%d\n", PORT);

    // 主循环：接受并处理连接
    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket < 0)
        {
            perror("接受客户端连接失败");
            continue;
        }

        // 创建子进程处理请求
        pid_t pid = fork();
        if (pid == 0)
        {
            // 子进程
            close(server_socket);
            handle_http_request(client_socket);
            close(client_socket);
            exit(0);
        }
        else
        {
            // 父进程
            close(client_socket);
            // 避免僵尸进程
            signal(SIGCHLD, SIG_IGN);
        }
    }

    close(server_socket);
    return 0;
}