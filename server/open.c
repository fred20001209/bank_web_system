#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> //creat函数
#include <unistd.h>

#define TYPE_OPEN 11
#define DATA_DIR "./bank_data/"
#define ID_FILE DATA_DIR "id.dat"

static int reqid = -1;
static int resid = -1;

typedef struct tag_Account
{ // 存放账户信息的结构体
	int id;
	char name[256];
	char passwd[6];
	double balance;
} ACCOUNT;

typedef struct tag_OpenRequest
{ // 开户请求
	long type;
	pid_t pid;
	char name[256];
	char passwd[6];
	double balance;
} OPEN_REQUEST;

typedef struct tag_OpenRespond
{ // 开户响应
	long type;
	char error[512];
	int id;
} OPEN_RESPOND;

typedef struct tag_SaveRequest
{ // 存储请求
	long type;
	pid_t pid;
	int id;
	char name[256];
	double money;
} SAVE_REQUEST;

typedef struct tag_SaveRespond
{ // 存储响应
	long type;
	char error[512];
	double balance;
} SAVE_RESPOND;

int save(const ACCOUNT *acc)
{
	// 确保数据目录存在
	struct stat st = {0};
	if (stat(DATA_DIR, &st) == -1)
	{
		if (mkdir(DATA_DIR, 0700) == -1)
		{
			perror("mkdir");
			return -1;
		}
	}

	char pathname[256];
	snprintf(pathname, sizeof(pathname), DATA_DIR "%d.acc", acc->id);

	int fd = creat(pathname, 0600); // 更严格的权限
	if (fd == -1)
	{
		perror("creat");
		return -1;
	}

	if (write(fd, acc, sizeof(*acc)) == -1)
	{
		perror("write");
		close(fd); // 确保资源释放
		return -1;
	}
	close(fd);
	return 0;
}

int genid(void)
{
	int id = 10;
	struct flock lock;

	// 确保目录存在
	struct stat st = {0};
	if (stat(DATA_DIR, &st) == -1)
	{
		if (mkdir(DATA_DIR, 0700) == -1)
		{
			perror("mkdir");
			return -1;
		}
	}

	int fd = open(ID_FILE, O_RDWR | O_CREAT, 0600);
	if (fd == -1)
	{
		perror("open");
		return -1;
	}

	// 设置文件锁
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if (fcntl(fd, F_SETLK, &lock) == -1)
	{
		perror("加锁失败");
		close(fd);
		return -1;
	}

	// 文件可能为空
	if (read(fd, &id, sizeof(id)) <= 0)
	{
		// 使用默认值id=10
		id = 10;
	}

	id++;

	if (lseek(fd, 0, SEEK_SET) == -1)
	{
		perror("lseek");
		goto cleanup;
	}

	if (write(fd, &id, sizeof(id)) == -1)
	{
		perror("write");
		goto cleanup;
	}

	// 解锁并关闭文件
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
	return id;

cleanup:
	lock.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &lock);
	close(fd);
	return -1;
}

void cleanup_resources(void)
{
	printf("开户服务正在清理资源...\n");
	// 消息队列由server负责清理
}

void sigint(int signum)
{
	printf("开户服务接收到信号%d，即将停止...\n", signum);
	cleanup_resources();
	exit(0);
}

int main(void)
{

	atexit(cleanup_resources);

	if (signal(SIGINT, sigint) == SIG_ERR) // 如果捕捉到SIGINT信号，则执行sigint函数，如果返回值为-1或为SIGERR,则出错。用来关闭开户进程
	{
		perror("signal");
		return -1;
	}

	key_t KEY_REQUEST = ftok("/home", 2); // 客户端键值        C语言初始化一个全局变量或static变量时，只能用常量赋值，不能用变量赋值！
	if (KEY_REQUEST == -1)
	{
		perror("ftok REQUEST");
		return -1;
	}

	key_t KEY_RESPOND = ftok("/home", 3); // 服务端键值
	if (KEY_RESPOND == -1)
	{
		perror("ftok RESPOND");
		return -1;
	}

	reqid = msgget(KEY_REQUEST, IPC_CREAT | 0600); // 将创建的消息队列请求ID赋值给发出请求开户的ID
	if (reqid == -1)							   // 如果为-1,说明创建失败
	{
		perror("msgget request");
		return -1;
	}

	resid = msgget(KEY_RESPOND, IPC_CREAT | 0600); // 获取响应开户的ID
	if (resid == -1)
	{
		perror("msgget respond");
		return -1;
	}

	printf("开户服务：启动就绪。\n"); // 说明我拿到了服务端消息队列中请求和响应的ID号，我可以利用这两个ID号向两个消息队列里写和取信息

	while (1)
	{
		OPEN_REQUEST req; // 结构体重命名，存放开户客户端信息

		if (msgrcv(reqid, &req, sizeof(req) - sizeof(req.type), TYPE_OPEN, 0) == -1) // 如果获取消息返回值为-1,说明信息获取失败
		{
			perror("msgrcv");
			sleep(1); // 避免CPU占用过高
			continue;
		}

		OPEN_RESPOND res = {req.pid, ""}; // 存放开户服务端信息。
		ACCOUNT acc;					  // 存放账户信息的结构体

		// 输入检查
		if (strlen(req.name) == 0)
		{
			sprintf(res.error, "账户名不能为空");
			goto send_respond;
		}

		if (req.balance <= 0)
		{
			sprintf(res.error, "初始存款必须大于0");
			goto send_respond;
		}

				if ((acc.id = genid()) == -1)
		{
			sprintf(res.error, "创建账户失败！\n"); // 向开户响应结构体传递账户创建失败的信息
			goto send_respond;
		}

		strcpy(acc.name, req.name); // 账户创建成功时，将客户端输入的名字，密码，金额信息放入账户结构体中
		strcpy(acc.passwd, req.passwd);
		acc.balance = req.balance;

		if (save(&acc) == -1)
		{
			sprintf(res.error, "保存账户失败！");
			goto send_respond;
		}

		res.id = acc.id; // 帐号保存成功，开户服务端获取这个账户的ID号
		printf("创建账户成功：ID=%d, 名称=%s\n", acc.id, acc.name);

	send_respond:

		if (msgsnd(resid, &res, sizeof(res) - sizeof(res.type), 0) == -1) // 向响应消息队列发送开户信息
		{
			perror("msgsnd");
			continue;
		}

		// 添加错误日志记录
		if (strlen(res.error) > 0)
		{
			printf("开户失败：%s\n", res.error);
		}
	}

	return 0;
}