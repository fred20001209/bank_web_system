CC = gcc
CFLAGS = -Wall -g

# 目标文件
TARGETS = server web_server open close save withdraw query transfer
BIN_DIR = ../bin

# 确保bin目录存在
$(shell mkdir -p $(BIN_DIR))

all: $(TARGETS)

# 原始银行系统服务器
server: server.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	# 在源码目录创建符号链接，保持原始行为
	ln -sf $(BIN_DIR)/$@ $@

# HTTP网络服务器
web_server: web_server.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	# 在源码目录创建符号链接，保持原始行为
	ln -sf $(BIN_DIR)/$@ $@

# 各个业务模块
open: open.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	# 在源码目录创建符号链接，保持原始行为
	ln -sf $(BIN_DIR)/$@ $@

close: close.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	ln -sf $(BIN_DIR)/$@ $@

save: save.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	ln -sf $(BIN_DIR)/$@ $@

withdraw: withdraw.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	ln -sf $(BIN_DIR)/$@ $@

query: query.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	ln -sf $(BIN_DIR)/$@ $@

transfer: transfer.c
	$(CC) $(CFLAGS) -o $(BIN_DIR)/$@ $<
	@echo "编译完成: $@"
	ln -sf $(BIN_DIR)/$@ $@

clean:
	rm -f $(addprefix $(BIN_DIR)/, $(TARGETS))
	# 清理符号链接
	rm -f $(TARGETS)