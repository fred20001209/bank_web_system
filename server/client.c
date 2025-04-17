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

#define TYPE_OPEN 11
#define TYPE_CLOSE 12
#define TYPE_SAVE 13
#define TYPE_WITHDRAW 14
#define TYPE_QUERY 15
#define TYPE_TRANSFER 16 // 消息类型

static int g_reqid = -1; // 请求消息队列ID初值
static int g_resid = -1; // 响应消息队列ID初值

typedef struct tag_OpenRequest
{			   // 开户请求
	long type; // 消息类型标识
	pid_t pid; // 客户端进程ID
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

typedef struct tag_CloseRequest
{ // 销户请求
	long type;
	pid_t pid;
	int id; // 要销户的账号
	char name[256];
	char passwd[6];
} CLOSE_REQUEST;

typedef struct tag_CloseRespond
{ // 销户响应
	long type;
	char error[512];
	double balance;
} CLOSE_RESPOND;

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

typedef struct tag_WithdrawRequest
{ // 取款请求
	long type;
	pid_t pid;
	int id;
	char name[256];
	char passwd[6];
	double money;
} WITHDRAW_REQUEST;

typedef struct tag_WithdrawRespond
{ // 取款响应
	long type;
	char error[512];
	double balance;
} WITHDRAW_RESPOND;

typedef struct tag_QueryRequest // 查询请求
{
	long type;
	pid_t pid;
	int id;
	char name[256];
	char passwd[6];
} QUERY_REQUEST;

typedef struct tag_QueryRespond // 查询响应
{
	long type;
	char error[512];
	double balance;
} QUERY_RESPOND;

typedef struct tag_TransferRequest // 转账请求
{
	long type;
	pid_t pid;
	int id[2];		   // 两个账号：源账户和目标账户
	char name[2][256]; // 两个账户名
	char passwd[6];	   // 源账户密码
	double money;	   // 转账金额
} TRANSFER_REQUEST;

typedef struct tag_TransferRespond // 转账响应
{
	long type;
	char error[512];
	double balance;
} TRANSFER_RESPOND;

void mmenu(void)
{
	printf("***欢迎来到银行系统***\n");
	printf("      1. 开户         \n");
	printf("      2. 销户         \n");
	printf("      3. 存款         \n");
	printf("      4. 取款         \n");
	printf("      5. 查询         \n");
	printf("      6. 转账         \n");
	printf("******0. 退出*********\n");
	printf("请选择你要操作的序号：");

	return;
}

int mquit(void) // 退出银行系统
{
	printf("欢迎下次使用！\n");
	return 0;
}

int mopen(void) // 开户
{
	pid_t pid = getpid();				 // 获取进程号
	OPEN_REQUEST req = {TYPE_OPEN, pid}; // 开户客户端：1类消息，进程号

	// 1. 安全的输入处理
	char temp_name[300]; // 使用更大的临时缓冲区
	printf("账户名(1-255字符)：");
	scanf("%299s", temp_name); // 限制读取长度

	if (strlen(temp_name) > 255)
	{
		printf("错误：账户名过长，请重新操作\n");
		return 0;
	}
	strcpy(req.name, temp_name);

	// 2. 密码格式验证
	char temp_passwd[20];
	printf("密码(6位数字)：");
	scanf("%19s", temp_passwd);

	if (strlen(temp_passwd) != 6)
	{
		printf("错误：密码必须为6位数字\n");
		return 0;
	}
	for (int i = 0; i < 6; i++)
	{
		if (temp_passwd[i] < '0' || temp_passwd[i] > '9')
		{
			printf("错误：密码必须全为数字\n");
			return 0;
		}
	}
	strcpy(req.passwd, temp_passwd);

	// 3. 存款金额验证
	printf("存款金额(必须大于0)：");
	scanf("%lf", &req.balance);
	if (req.balance <= 0)
	{
		printf("错误：存款金额必须大于0\n");
		return 0;
	}

	// 4. 消息发送处理
	printf("正在处理开户请求...\n");
	if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(req.type), 0) == -1)
	{
		perror("发送开户请求失败");
		return 0;
	}

	// 5. 接收并验证响应
	OPEN_RESPOND res; // 开户服务端结构体

	if (msgrcv(g_resid, &res, sizeof(res) - sizeof(res.type), pid, 0) == -1)
	{
		perror("接收开户响应失败");
		return 0;
	}

	// 6. 检查错误信息
	if (strlen(res.error) > 0)
	{
		printf("开户失败：%s\n", res.error);
		return 0;
	}

	printf("恭喜，开户成功！您的账号是：%d\n", res.id);
	return 0;
}

int mclose(void) // 销户
{
	// key_t KEY_REQUEST = ftok("/home", 2); // 客户端键值
	// key_t KEY_RESPOND = ftok("/home", 3); // 服务端键值

	pid_t pid = getpid(); // 获取进程号
	CLOSE_REQUEST req = {TYPE_CLOSE, pid};

	// 1. 账号输入和验证
	printf("请输入要销户的账号：");
	if (scanf("%d", &req.id) != 1 || req.id <= 0)
	{
		printf("错误：账号必须为正整数\n");
		while (getchar() != '\n')
			; // 清空输入缓冲区
		return 0;
	}

	// 2. 安全的账户名输入
	char temp_name[300];
	printf("请输入账户名：");
	scanf("%299s", temp_name);
	if (strlen(temp_name) > 255)
	{
		printf("错误：账户名过长\n");
		return 0;
	}
	strcpy(req.name, temp_name);

	// 3. 密码验证
	char temp_passwd[20];
	printf("请输入密码：");
	scanf("%19s", temp_passwd);
	if (strlen(temp_passwd) != 6)
	{
		printf("错误：密码必须为6位数字\n");
		return 0;
	}
	strcpy(req.passwd, temp_passwd);

	// 4. 确认销户意图
	printf("警告：销户操作不可撤销！确认销户请输入Y，取消请输入N：");
	char confirm;
	scanf(" %c", &confirm);
	if (confirm != 'Y' && confirm != 'y')
	{
		printf("销户操作已取消\n");
		return 0;
	}

	// 5. 发送请求
	printf("正在处理销户请求...\n");
	if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(req.type), 0) == -1)
	{
		perror("发送销户请求失败");
		return 0;
	}

	// 6. 接收响应
	CLOSE_RESPOND res;
	if (msgrcv(g_resid, &res, sizeof(res) - sizeof(res.type), pid, 0) == -1)
	{
		perror("接收销户响应失败");
		return 0;
	}

	// 7. 错误处理
	if (strlen(res.error) > 0)
	{
		printf("销户失败：%s\n", res.error);
		return 0;
	}

	printf("销户成功！最终账户余额：%.2lf元已退还\n", res.balance);
	return 0;
}

int msave(void) // 存款
{
	pid_t pid = getpid();
	SAVE_REQUEST req = {TYPE_SAVE, pid};

	// 1. 账号输入与验证
	printf("请输入账号：");
	if (scanf("%d", &req.id) != 1 || req.id <= 0)
	{
		printf("错误：账号必须为正整数\n");
		while (getchar() != '\n')
			; // 清空输入缓冲区
		return 0;
	}

	// 2. 安全的账户名输入
	char temp_name[300];
	printf("请输入账户名：");
	scanf("%299s", temp_name);
	if (strlen(temp_name) > 255)
	{
		printf("错误：账户名过长\n");
		return 0;
	}
	strcpy(req.name, temp_name);

	// 3. 存款金额验证
	printf("请输入存款金额（大于0）：");
	scanf("%lf", &req.money);
	if (req.money <= 0)
	{
		printf("错误：存款金额必须大于0\n");
		return 0;
	}

	// 4. 确认存款信息
	printf("\n--存款信息确认--\n");
	printf("账号：%d\n", req.id);
	printf("账户名：%s\n", req.name);
	printf("存款金额：%.2f元\n", req.money);
	printf("确认存款请按Y，取消请按N：");

	char confirm;
	scanf(" %c", &confirm);
	if (confirm != 'Y' && confirm != 'y')
	{
		printf("存款操作已取消\n");
		return 0;
	}

	// 5. 发送请求
	printf("正在处理存款请求...\n");
	if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(req.type), 0) == -1)
	{
		perror("发送存款请求失败");
		return 0;
	}

	// 6. 接收响应
	SAVE_RESPOND res;
	if (msgrcv(g_resid, &res, sizeof(res) - sizeof(res.type), pid, 0) == -1)
	{
		perror("接收存款响应失败");
		return 0;
	}

	// 7. 处理错误信息
	if (strlen(res.error) > 0)
	{
		printf("存款失败：%s\n", res.error);
		return 0;
	}

	// 8. 显示存款成功信息
	printf("存款成功！当前账户余额：%.2lf元\n", res.balance);
	return 0;
}

int mwithdraw(void) // 取款
{
	pid_t pid = getpid();
	WITHDRAW_REQUEST req = {TYPE_WITHDRAW, pid};

	// 1. 账号输入与验证
	printf("请输入账号：");
	if (scanf("%d", &req.id) != 1 || req.id <= 0)
	{
		printf("错误：账号必须为正整数\n");
		while (getchar() != '\n')
			; // 清空输入缓冲区
		return 0;
	}

	// 2. 安全的账户名输入
	char temp_name[300];
	printf("请输入账户名：");
	scanf("%299s", temp_name);
	if (strlen(temp_name) > 255)
	{
		printf("错误：账户名过长\n");
		return 0;
	}
	strcpy(req.name, temp_name);

	// 3. 密码验证
	char temp_passwd[20];
	printf("请输入密码(6位数字)：");
	scanf("%19s", temp_passwd);
	if (strlen(temp_passwd) != 6)
	{
		printf("错误：密码必须为6位数字\n");
		return 0;
	}
	for (int i = 0; i < 6; i++)
	{
		if (temp_passwd[i] < '0' || temp_passwd[i] > '9')
		{
			printf("错误：密码必须全为数字\n");
			return 0;
		}
	}
	strcpy(req.passwd, temp_passwd);

	// 4. 取款金额验证
	printf("请输入取款金额（大于0）：");
	scanf("%lf", &req.money);
	if (req.money <= 0)
	{
		printf("错误：取款金额必须大于0\n");
		return 0;
	}

	// 5. 确认取款信息
	printf("\n--取款信息确认--\n");
	printf("账号：%d\n", req.id);
	printf("账户名：%s\n", req.name);
	printf("取款金额：%.2f元\n", req.money);
	printf("确认取款请按Y，取消请按N：");

	char confirm;
	scanf(" %c", &confirm);
	if (confirm != 'Y' && confirm != 'y')
	{
		printf("取款操作已取消\n");
		return 0;
	}

	// 6. 发送请求
	printf("正在处理取款请求...\n");
	if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(req.type), 0) == -1)
	{
		perror("发送取款请求失败");
		return 0;
	}

	// 7. 接收响应
	WITHDRAW_RESPOND res;
	if (msgrcv(g_resid, &res, sizeof(res) - sizeof(res.type), pid, 0) == -1)
	{
		perror("接收取款响应失败");
		return 0;
	}

	// 8. 处理错误信息
	if (strlen(res.error) > 0)
	{
		printf("取款失败：%s\n", res.error);
		return 0;
	}

	// 9. 显示取款成功信息
	printf("取款成功！取出金额：%.2lf元，当前账户余额：%.2lf元\n",
		   req.money, res.balance);
	return 0;
}

int mquery(void) // 查询
{
	pid_t pid = getpid();
	QUERY_REQUEST req = {TYPE_QUERY, pid};

	// 1. 账号输入与验证
	printf("请输入账号：");
	if (scanf("%d", &req.id) != 1 || req.id <= 0)
	{
		printf("错误：账号必须为正整数\n");
		while (getchar() != '\n')
			; // 清空输入缓冲区
		return 0;
	}

	// 2. 安全的账户名输入
	char temp_name[300];
	printf("请输入账户名：");
	scanf("%299s", temp_name);
	if (strlen(temp_name) > 255)
	{
		printf("错误：账户名过长\n");
		return 0;
	}
	strcpy(req.name, temp_name);

	// 3. 密码安全输入和验证
	char temp_passwd[20];
	printf("请输入密码(6位数字)：");
	scanf("%19s", temp_passwd);
	if (strlen(temp_passwd) != 6)
	{
		printf("错误：密码必须为6位数字\n");
		return 0;
	}
	for (int i = 0; i < 6; i++)
	{
		if (temp_passwd[i] < '0' || temp_passwd[i] > '9')
		{
			printf("错误：密码必须全为数字\n");
			return 0;
		}
	}
	strcpy(req.passwd, temp_passwd);

	// 4. 显示正在处理的提示
	printf("\n正在查询账户信息...\n");

	// 5. 发送请求
	if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(req.type), 0) == -1)
	{
		perror("发送查询请求失败");
		return 0;
	}

	// 6. 接收响应
	QUERY_RESPOND res;
	if (msgrcv(g_resid, &res, sizeof(res) - sizeof(res.type), pid, 0) == -1)
	{
		perror("接收查询响应失败");
		return 0;
	}

	// 7. 处理错误信息
	if (strlen(res.error) > 0)
	{
		printf("查询失败：%s\n", res.error);
		return 0;
	}

	// 8. 格式化显示查询结果
	printf("\n====== 账户查询结果 ======\n");
	printf("账号：%d\n", req.id);
	printf("账户名：%s\n", req.name);
	printf("当前余额：%.2lf元\n", res.balance);
	printf("========================\n");

	return 0;
}

int mtransfer(void) // 转账
{
	pid_t pid = getpid();
	TRANSFER_REQUEST req = {TYPE_TRANSFER, pid};

	// 1. 源账号输入与验证
	printf("请输入己方账号：");
	if (scanf("%d", &req.id[0]) != 1 || req.id[0] <= 0)
	{
		printf("错误：账号必须为正整数\n");
		while (getchar() != '\n')
			; // 清空输入缓冲区
		return 0;
	}

	// 2. 源账户名安全输入
	char temp_name1[300];
	printf("请输入己方账户名：");
	scanf("%299s", temp_name1);
	if (strlen(temp_name1) > 255)
	{
		printf("错误：账户名过长\n");
		return 0;
	}
	strcpy(req.name[0], temp_name1);

	// 3. 密码安全输入和验证
	char temp_passwd[20];
	printf("请输入己方密码(6位数字)：");
	scanf("%19s", temp_passwd);
	if (strlen(temp_passwd) != 6)
	{
		printf("错误：密码必须为6位数字\n");
		return 0;
	}
	for (int i = 0; i < 6; i++)
	{
		if (temp_passwd[i] < '0' || temp_passwd[i] > '9')
		{
			printf("错误：密码必须全为数字\n");
			return 0;
		}
	}
	strcpy(req.passwd, temp_passwd);

	// 4. 转账金额验证
	printf("请输入转账金额（大于0）：");
	scanf("%lf", &req.money);
	if (req.money <= 0)
	{
		printf("错误：转账金额必须大于0\n");
		return 0;
	}

	// 设置合理的最大转账金额
	const double MAX_TRANSFER = 50000.0; // 根据业务需求设置
	if (req.money > MAX_TRANSFER)
	{
		printf("错误：单笔转账超出最大限额(%.2lf元)\n", MAX_TRANSFER);
		return 0;
	}

	// 5. 目标账号输入与验证
	printf("请输入对方账号：");
	if (scanf("%d", &req.id[1]) != 1 || req.id[1] <= 0)
	{
		printf("错误：账号必须为正整数\n");
		while (getchar() != '\n')
			; // 清空输入缓冲区
		return 0;
	}

	// 检查不能转给自己
	if (req.id[0] == req.id[1])
	{
		printf("错误：不能转账给自己\n");
		return 0;
	}

	// 6. 目标账户名安全输入
	char temp_name2[300];
	printf("请输入对方账户名：");
	scanf("%299s", temp_name2);
	if (strlen(temp_name2) > 255)
	{
		printf("错误：账户名过长\n");
		return 0;
	}
	strcpy(req.name[1], temp_name2);

	// 7. 转账信息确认（重要！）
	printf("\n====== 转账信息确认 ======\n");
	printf("从己方账号：%d (%s)\n", req.id[0], req.name[0]);
	printf("转入对方账号：%d (%s)\n", req.id[1], req.name[1]);
	printf("转账金额：%.2lf元\n", req.money);
	printf("请仔细核对以上信息！\n");
	printf("确认转账请输入Y，取消请输入N：");

	char confirm;
	scanf(" %c", &confirm);
	if (confirm != 'Y' && confirm != 'y')
	{
		printf("转账操作已取消\n");
		return 0;
	}

	// 8. 二次确认（对于大额转账）
	if (req.money >= 10000.0)
	{
		printf("\n注意：当前为大额转账(%.2lf元)，再次确认请输入Y，取消请输入N：", req.money);
		scanf(" %c", &confirm);
		if (confirm != 'Y' && confirm != 'y')
		{
			printf("转账操作已取消\n");
			return 0;
		}
	}

	// 9. 发送请求
	printf("正在处理转账请求，请稍候...\n");
	if (msgsnd(g_reqid, &req, sizeof(req) - sizeof(req.type), 0) == -1)
	{
		perror("发送转账请求失败");
		return 0;
	}

	// 10. 接收响应
	TRANSFER_RESPOND res;
	if (msgrcv(g_resid, &res, sizeof(res) - sizeof(res.type), pid, 0) == -1)
	{
		perror("接收转账响应失败");
		return 0;
	}

	// 11. 处理错误信息
	if (strlen(res.error) > 0)
	{
		printf("转账失败：%s\n", res.error);
		return 0;
	}

	// 12. 显示转账成功信息
	printf("\n====== 转账成功 ======\n");
	printf("已成功向账号%d(%s)转账%.2lf元\n",
		   req.id[1], req.name[1], req.money);
	printf("己方账户当前余额：%.2lf元\n", res.balance);
	printf("交易时间：%s\n", __DATE__ " " __TIME__);
	printf("====================\n");

	return 0;
}

// 添加资源清理函数
void cleanup_resources()
{
	printf("正在清理系统资源...\n");
	// 不删除队列，因为服务端可能仍在使用
}

// 添加信号处理
void handle_signal(int sig)
{
	printf("\n接收到中断信号，正在退出系统\n");
	cleanup_resources();
	exit(0);
}

int main(void)
{

	// 注册信号处理函数
	signal(SIGINT, handle_signal);

	// 注册退出清理函数
	atexit(cleanup_resources);

	key_t KEY_REQUEST = ftok("/home", 2); // 客户端键值
	if (KEY_REQUEST == -1)
	{
		perror("ftok请求队列失败");
		return -1;
	}
	key_t KEY_RESPOND = ftok("/home", 3); // 服务端键值
	if (KEY_RESPOND == -1)
	{
		perror("ftok响应队列失败");
		return -1;
	}

	if ((g_reqid = msgget(KEY_REQUEST, IPC_CREAT | 0600)) == -1)
	{
		perror("msgget");
		return -1;
	}

	if ((g_resid = msgget(KEY_RESPOND, IPC_CREAT | 0600)) == -1)
	{
		perror("msgget");
		return -1;
	}
	while (1)
	{
		mmenu();
		int choice;
		printf("请选择: ");
		if (scanf("%d", &choice) != 1)
		{
			printf("输入无效，请输入数字\n");
			while (getchar() != '\n')
				; // 清空输入缓冲区
			continue;
		}
		switch (choice)
		{
		case 1:
			mopen();
			break;
		case 2:
			mclose();
			break;
		case 3:
			msave();
			break;
		case 4:
			mwithdraw();
			break;
		case 5:
			mquery();
			break;
		case 6:
			mtransfer();
			break;
		case 0:
			mquit();
			return 0;
		default:
			printf("输入错误，请重新输入！\n");
		}
	}
	return 0;
}
