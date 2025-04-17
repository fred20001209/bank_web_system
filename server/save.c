#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define TYPE_SAVE 13			// 存款消息类型
#define DATA_DIR "./bank_data/" // 统一数据目录

// 全局变量
static int reqid = -1;
static int resid = -1;

typedef struct tag_Account
{					// 存放账户信息的结构体
	int id;			// 账号
	char name[256]; // 账户名
	char passwd[6]; // 账户密码
	double balance; // 账户金额
} ACCOUNT;

typedef struct tag_SaveRequest
{ // 存款请求
	long type;
	pid_t pid;
	int id;
	char name[256];
	double money;
} SAVE_REQUEST;

typedef struct tag_SaveRespond
{ // 存款响应
	long type;
	char error[512];
	double balance;
} SAVE_RESPOND;

int get(int id, ACCOUNT *acc)
{
	char pathname[256];
	snprintf(pathname, sizeof(pathname), DATA_DIR "%d.acc", id);

	int fd = open(pathname, O_RDONLY);
	if (fd == -1)
	{
		perror("open");
		return -1;
	}

	if (read(fd, acc, sizeof(*acc)) == -1)
	{
		perror("read");
		close(fd); // 确保关闭文件
		return -1;
	}
	close(fd);
	return 0;
}

int update(const ACCOUNT *acc) // 更新账户信息
{
	char pathname[256]; // 账户文件
	snprintf(pathname, sizeof(pathname), DATA_DIR "%d.acc", acc->id);

	int fd = open(pathname, O_WRONLY); // 只写的方式打开账户文件
	if (fd == -1)
	{
		perror("open"); // 打开文件失败
		return -1;
	}

	if (write(fd, acc, sizeof(*acc)) == -1) // 向文件中写入内容
	{
		perror("write"); // 写入内容失败
		close(fd);		 // 确保关闭文件
		return -1;
	}
	close(fd); // 关闭文件
	return 0;
}

// 添加资源清理函数
void cleanup_resources(void)
{
	printf("存款服务正在清理资源...\n");
	// 消息队列由server负责清理
}

void sigint(int signum) // 信号处理函数
{
	printf("存款服务接收到信号%d，即将停止...\n", signum);
	cleanup_resources();
	exit(0);
}

int main(void)
{
	// 注册退出清理函数
	atexit(cleanup_resources);

	if (signal(SIGINT, sigint) == SIG_ERR) // 注册信号处理
	{
		perror("signal");
		return -1;
	}

	key_t KEY_REQUEST = ftok("/home", 2); // 客户端键值
	key_t KEY_RESPOND = ftok("/home", 3); // 服务端键值

	// 检查键值生成是否成功
	if (KEY_REQUEST == -1 || KEY_RESPOND == -1)
	{
		perror("ftok");
		return -1;
	}

	// 使用全局变量存储消息队列ID，并使用更安全的权限
	reqid = msgget(KEY_REQUEST, IPC_CREAT | 0600);
	if (reqid == -1)
	{
		perror("msgget");
		return -1;
	}

	resid = msgget(KEY_RESPOND, IPC_CREAT | 0600);
	if (resid == -1)
	{
		perror("msgget");
		return -1;
	}

	// 确保数据目录存在
	struct stat st = {0};
	if (stat(DATA_DIR, &st) == -1)
	{
		if (mkdir(DATA_DIR, 0700) == -1)
		{
			perror("mkdir");
			// 不退出，因为目录可能已存在
		}
	}

	printf("存款服务：启动就绪。\n");

	while (1) // 接收消息
	{
		SAVE_REQUEST req; // 存款客户端结构体

		if (msgrcv(reqid, &req, sizeof(req) - sizeof(req.type), TYPE_SAVE, 0) == -1)
		{
			if (errno == EINTR)
			{
				// 被信号中断，继续接收
				continue;
			}
			perror("msgrcv");
			sleep(1); // 避免CPU占用过高
			continue;
		}

		SAVE_RESPOND res = {req.pid, ""}; // 存款服务端结构体
		ACCOUNT acc;					  // 账户结构体

		// 添加基本输入验证
		if (req.id <= 0)
		{
			sprintf(res.error, "无效账号：账号必须为正整数");
			goto send_respond;
		}

		if (req.money <= 0)
		{
			sprintf(res.error, "存款金额必须大于0");
			goto send_respond;
		}

		if (get(req.id, &acc) == -1)
		{
			sprintf(res.error, "无效账号！");
			goto send_respond;
		}

		if (strcmp(req.name, acc.name)) // 验证账户名
		{
			sprintf(res.error, "无效户名！");
			goto send_respond;
		}

		acc.balance += req.money; // 增加存款金额

		if (update(&acc) == -1) // 更新账户信息
		{
			sprintf(res.error, "更新账户失败！");
			goto send_respond;
		}

		res.balance = acc.balance; // 返回更新后的余额
		printf("存款成功：账号=%d, 存入金额=%.2f, 当前余额=%.2f\n",
			   req.id, req.money, acc.balance);

	send_respond:
		if (msgsnd(resid, &res, sizeof(res) - sizeof(res.type), 0) == -1)
		{
			perror("msgsnd");
			continue;
		}

		// 添加错误日志记录
		if (strlen(res.error) > 0)
		{
			printf("存款失败: %s\n", res.error);
		}
	}

	return 0;
}