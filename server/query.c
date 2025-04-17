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

#define TYPE_QUERY 15
#define DATA_DIR "./bank_data/" // 统一数据目录

// 全局变量，方便信号处理函数使用
static int reqid = -1;
static int resid = -1;

typedef struct tag_Account
{
	int id;
	char name[256];
	char passwd[6]; // 修改为6字节，与其他模块保持一致
	double balance;
} ACCOUNT;

typedef struct tag_QueryRequest
{
	long type;
	pid_t pid;
	int id;
	char name[256];
	char passwd[6]; // 修改为6字节
} QUERY_REQUEST;

typedef struct tag_QueryRespond
{
	long type;
	char error[512];
	double balance;
} QUERY_RESPOND;

int get(int id, ACCOUNT *acc)
{
	// 确保使用统一的数据目录
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

// 添加资源清理函数
void cleanup_resources(void)
{
	printf("查询服务正在清理资源...\n");
	// 消息队列由server负责清理
}

void sigint(int signum)
{
	printf("查询服务接收到信号%d，即将停止...\n", signum);
	cleanup_resources();
	exit(0);
}

int main(void)
{
	// 注册退出清理函数
	atexit(cleanup_resources);

	// 注册信号处理
	if (signal(SIGINT, sigint) == SIG_ERR)
	{
		perror("signal");
		return -1;
	}

	key_t KEY_REQUEST = ftok("/home", 2);
	key_t KEY_RESPOND = ftok("/home", 3);

	// 检查键值生成是否成功
	if (KEY_REQUEST == -1 || KEY_RESPOND == -1)
	{
		perror("ftok");
		return -1;
	}

	// 使用全局变量存储消息队列ID，使用更严格的权限
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

	printf("查询服务：启动就绪。\n");

	while (1)
	{
		QUERY_REQUEST req;

		if (msgrcv(reqid, &req, sizeof(req) - sizeof(req.type), TYPE_QUERY, 0) == -1)
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

		QUERY_RESPOND res = {req.pid, ""};
		ACCOUNT acc;

		// 输入验证
		if (req.id <= 0)
		{
			sprintf(res.error, "无效账号: 账号必须为正整数");
			goto send_respond;
		}

		if (strlen(req.name) == 0)
		{
			sprintf(res.error, "账户名不能为空");
			goto send_respond;
		}

		if (get(req.id, &acc) == -1)
		{
			sprintf(res.error, "无效账号！");
			goto send_respond;
		}

		if (strcmp(req.name, acc.name))
		{
			sprintf(res.error, "无效户名！");
			goto send_respond;
		}

		if (strcmp(req.passwd, acc.passwd))
		{
			sprintf(res.error, "密码错误！");
			goto send_respond;
		}

		res.balance = acc.balance;
		printf("账户查询成功: ID=%d, 余额=%.2f\n", req.id, acc.balance);

	send_respond:
		if (msgsnd(resid, &res, sizeof(res) - sizeof(res.type), 0) == -1)
		{
			perror("msgsnd");
			continue;
		}

		// 记录操作结果
		if (strlen(res.error) > 0)
		{
			printf("查询失败: %s\n", res.error);
		}
	}

	return 0;
}