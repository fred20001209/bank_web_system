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

#define TYPE_TRANSFER 16
#define DATA_DIR "./bank_data/" // 添加统一数据目录

// 全局变量，方便信号处理函数使用
static int reqid = -1;
static int resid = -1;

typedef struct tag_Account
{
	int id;
	char name[256];
	char passwd[6]; // 改为6字节，与其他模块一致
	double balance;
} ACCOUNT;

typedef struct tag_TransferRequest
{
	long type;
	pid_t pid;
	int id[2];
	char name[2][256];
	char passwd[6]; // 改为6字节
	double money;
} TRANSFER_REQUEST;

typedef struct tag_TransferRespond
{
	long type;
	char error[512];
	double balance;
} TRANSFER_RESPOND;

int update(const ACCOUNT *acc)
{
	char pathname[256];
	snprintf(pathname, sizeof(pathname), DATA_DIR "%d.acc", acc->id);

	int fd = open(pathname, O_WRONLY);
	if (fd == -1)
	{
		perror("open");
		return -1;
	}

	if (write(fd, acc, sizeof(*acc)) == -1)
	{
		perror("write");
		close(fd); // 确保关闭文件
		return -1;
	}
	close(fd);
	return 0;
}

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

// 添加资源清理函数
void cleanup_resources(void)
{
	printf("转账服务正在清理资源...\n");
	// 消息队列由server负责清理
}

void sigint(int signum)
{
	printf("转账服务接收到信号%d，即将停止...\n", signum);
	cleanup_resources();
	exit(0);
}

int main(void)
{
	// 注册退出清理函数
	atexit(cleanup_resources);

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

	// 使用全局变量存储队列ID，并使用更安全的权限
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

	printf("转账服务：启动就绪。\n");

	while (1)
	{
		TRANSFER_REQUEST req;

		if (msgrcv(reqid, &req, sizeof(req) - sizeof(req.type), TYPE_TRANSFER, 0) == -1)
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

		TRANSFER_RESPOND res = {req.pid, ""};
		ACCOUNT acc[2];

		// 添加基本输入验证
		if (req.id[0] <= 0 || req.id[1] <= 0)
		{
			sprintf(res.error, "无效账号：账号必须为正整数");
			goto send_respond;
		}

		if (req.money <= 0)
		{
			sprintf(res.error, "转账金额必须大于0");
			goto send_respond;
		}

		if (req.id[0] == req.id[1])
		{
			sprintf(res.error, "不能向自己转账");
			goto send_respond;
		}

		if (get(req.id[0], &acc[0]) == -1)
		{
			sprintf(res.error, "无效账号！");
			goto send_respond;
		}

		if (strcmp(req.name[0], acc[0].name))
		{
			sprintf(res.error, "无效户名！");
			goto send_respond;
		}

		if (strcmp(req.passwd, acc[0].passwd))
		{
			sprintf(res.error, "密码错误！");
			goto send_respond;
		}

		if (req.money > acc[0].balance)
		{
			sprintf(res.error, "余额不足！");
			goto send_respond;
		}

		if (get(req.id[1], &acc[1]) == -1)
		{
			sprintf(res.error, "无效对方账号！");
			goto send_respond;
		}

		if (strcmp(req.name[1], acc[1].name))
		{
			sprintf(res.error, "无效对方户名！");
			goto send_respond;
		}

		// 记录原始余额，用于后续可能的恢复
		double original_balance = acc[0].balance;

		// 执行转账
		acc[0].balance -= req.money;

		if (update(&acc[0]) == -1)
		{
			sprintf(res.error, "更新账户失败！");
			goto send_respond;
		}

		acc[1].balance += req.money;

		if (update(&acc[1]) == -1)
		{
			// 第二步失败时恢复第一个账户
			acc[0].balance = original_balance;
			if (update(&acc[0]) == -1)
			{
				// 恢复也失败，记录严重错误
				sprintf(res.error, "严重错误：转账失败且无法恢复原状态！");
			}
			else
			{
				sprintf(res.error, "更新对方账户失败，已恢复您的余额！");
			}
			goto send_respond;
		}

		res.balance = acc[0].balance;
		printf("转账成功：从账号%d向账号%d转账%.2f元\n",
			   req.id[0], req.id[1], req.money);

	send_respond:
		if (msgsnd(resid, &res, sizeof(res) - sizeof(res.type), 0) == -1)
		{
			perror("msgsnd");
			continue;
		}

		// 添加错误日志记录
		if (strlen(res.error) > 0)
		{
			printf("转账失败: %s\n", res.error);
		}
	}

	return 0;
}