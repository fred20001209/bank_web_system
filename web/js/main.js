document.addEventListener('DOMContentLoaded', function () {
    // 获取所有操作按钮
    const tabBtns = document.querySelectorAll('.tab-btn');
    const tabPanes = document.querySelectorAll('.tab-pane');

    // 添加表单内容
    function setupForms() {
        // 开户表单
        document.getElementById('open').innerHTML = `
            <h2>开户</h2>
            <form id="openForm">
                <div class="form-group">
                    <label for="name">账户名（1-255字符）：</label>
                    <input type="text" id="name" name="name" required maxlength="255">
                    <div id="name-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="passwd">密码（6位数字）：</label>
                    <input type="password" id="passwd" name="passwd" maxlength="6" pattern="\\d{6}" required>
                    <div id="passwd-error" class="error-text"></div>
                    <small class="form-text">请输入6位数字密码</small>
                </div>
                <div class="form-group">
                    <label for="amount">初始存款（必须大于0）：</label>
                    <input type="number" id="amount" name="amount" step="0.01" min="0.01" required>
                    <div id="amount-error" class="error-text"></div>
                </div>
                <button type="submit" class="btn">提交开户申请</button>
            </form>
            <div class="result-container">
                <div id="openResult" class="result"></div>
            </div>
        `;

        // 销户表单
        document.getElementById('close').innerHTML = `
            <h2>销户</h2>
            <form id="closeForm">
                <div class="form-group">
                    <label for="close_account_id">账号：</label>
                    <input type="number" id="close_account_id" name="account_id" required min="1">
                    <div id="close-account-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="close_name">姓名：</label>
                    <input type="text" id="close_name" name="name" required maxlength="255">
                    <div id="close-name-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="close_passwd">密码（6位数字）：</label>
                    <input type="password" id="close_passwd" name="passwd" maxlength="6" required>
                    <div id="close-passwd-error" class="error-text"></div>
                    <small class="form-text">请输入6位数字密码</small>
                </div>
                <div class="confirm-section">
                    <p><strong>警告：</strong> 销户操作不可撤销，账户内余额将被提取</p>
                    <button type="submit" class="btn btn-danger">确认销户</button>
                </div>
            </form>
            <div class="result-container">
                <div id="closeResult" class="result"></div>
            </div>
        `;

        // 存款表单
        document.getElementById('save').innerHTML = `
            <h2>存款</h2>
            <form id="saveForm">
                <div class="form-group">
                    <label for="save_account_id">账号：</label>
                    <input type="number" id="save_account_id" name="account_id" required min="1">
                    <div id="save-account-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="save_name">姓名：</label>
                    <input type="text" id="save_name" name="name" required maxlength="255">
                    <div id="save-name-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="save_amount">存款金额（必须大于0）：</label>
                    <input type="number" id="save_amount" name="amount" step="0.01" min="0.01" required>
                    <div id="save-amount-error" class="error-text"></div>
                </div>
                <button type="submit" class="btn">确认存款</button>
            </form>
            <div class="result-container">
                <div id="saveResult" class="result"></div>
            </div>
        `;

        // 取款表单
        document.getElementById('withdraw').innerHTML = `
            <h2>取款</h2>
            <form id="withdrawForm">
                <div class="form-group">
                    <label for="withdraw_account_id">账号：</label>
                    <input type="number" id="withdraw_account_id" name="account_id" required min="1">
                    <div id="withdraw-account-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="withdraw_name">姓名：</label>
                    <input type="text" id="withdraw_name" name="name" required maxlength="255">
                    <div id="withdraw-name-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="withdraw_passwd">密码（6位数字）：</label>
                    <input type="password" id="withdraw_passwd" name="passwd" maxlength="6" required>
                    <div id="withdraw-passwd-error" class="error-text"></div>
                    <small class="form-text">请输入6位数字密码</small>
                </div>
                <div class="form-group">
                    <label for="withdraw_amount">取款金额（必须大于0）：</label>
                    <input type="number" id="withdraw_amount" name="amount" step="0.01" min="0.01" required>
                    <div id="withdraw-amount-error" class="error-text"></div>
                </div>
                <button type="submit" class="btn">确认取款</button>
            </form>
            <div class="result-container">
                <div id="withdrawResult" class="result"></div>
            </div>
        `;

        // 查询表单
        document.getElementById('query').innerHTML = `
            <h2>查询</h2>
            <form id="queryForm">
                <div class="form-group">
                    <label for="query_account_id">账号：</label>
                    <input type="number" id="query_account_id" name="account_id" required min="1">
                    <div id="query-account-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="query_name">姓名：</label>
                    <input type="text" id="query_name" name="name" required maxlength="255">
                    <div id="query-name-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="query_passwd">密码（6位数字）：</label>
                    <input type="password" id="query_passwd" name="passwd" maxlength="6" required>
                    <div id="query-passwd-error" class="error-text"></div>
                    <small class="form-text">请输入6位数字密码</small>
                </div>
                <button type="submit" class="btn">查询</button>
            </form>
            <div class="result-container">
                <div id="queryResult" class="result"></div>
            </div>
        `;

        // 转账表单
        document.getElementById('transfer').innerHTML = `
            <h2>转账</h2>
            <form id="transferForm">
                <div class="form-group">
                    <label for="transfer_account_id">己方账号：</label>
                    <input type="number" id="transfer_account_id" name="account_id" required min="1">
                    <div id="transfer-account-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="transfer_name">己方姓名：</label>
                    <input type="text" id="transfer_name" name="name" required maxlength="255">
                    <div id="transfer-name-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="transfer_passwd">己方密码（6位数字）：</label>
                    <input type="password" id="transfer_passwd" name="passwd" maxlength="6" required>
                    <div id="transfer-passwd-error" class="error-text"></div>
                    <small class="form-text">请输入6位数字密码</small>
                </div>
                <div class="form-group">
                    <label for="transfer_to_account">对方账号：</label>
                    <input type="number" id="transfer_to_account" name="to_account" required min="1">
                    <div id="transfer-to-account-error" class="error-text"></div>
                </div>
                <div class="form-group">
                    <label for="transfer_to_name">对方姓名：</label>
                    <input type="text" id="transfer_to_name" name="to_name" required maxlength="255">
                    <div id="transfer-to-name-error" class="error-text"></div>
                    <small class="form-text">请确保对方姓名准确无误</small>
                </div>
                <div class="form-group">
                    <label for="transfer_amount">转账金额（单笔上限50000元）：</label>
                    <input type="number" id="transfer_amount" name="amount" step="0.01" min="0.01" max="50000" required>
                    <div id="transfer-amount-error" class="error-text"></div>
                </div>
                <button type="submit" class="btn">确认转账</button>
            </form>
            <div class="result-container">
                <div id="transferResult" class="result"></div>
            </div>
        `;

        // 设置密码验证
        setupPasswordValidation('passwd', 'passwd-error');
        setupPasswordValidation('close_passwd', 'close-passwd-error');
        setupPasswordValidation('withdraw_passwd', 'withdraw-passwd-error');
        setupPasswordValidation('query_passwd', 'query-passwd-error');
        setupPasswordValidation('transfer_passwd', 'transfer-passwd-error');

        // 验证转账给自己
        setupTransferValidation();

        // 设置表单提交处理
        setupFormSubmit('open', 'openForm', 'openResult', displayOpenResult);
        setupFormSubmit('close', 'closeForm', 'closeResult', displayCloseResult);
        setupFormSubmit('save', 'saveForm', 'saveResult', displaySaveResult);
        setupFormSubmit('withdraw', 'withdrawForm', 'withdrawResult', displayWithdrawResult);
        setupFormSubmit('query', 'queryForm', 'queryResult', displayQueryResult);
        setupTransferFormSubmit();
    }

    // 设置密码验证
    function setupPasswordValidation(inputId, errorId) {
        const input = document.getElementById(inputId);
        const error = document.getElementById(errorId);

        if (input && error) {
            input.addEventListener('input', function () {
                validatePassword(this.value, error);
            });

            input.addEventListener('blur', function () {
                validatePassword(this.value, error);
            });
        }
    }

    // 验证密码格式
    function validatePassword(value, errorElement) {
        if (!value) {
            errorElement.textContent = '';
            errorElement.style.display = 'none';
            return true;
        }

        if (value.length !== 6) {
            errorElement.textContent = '密码必须为6位数字';
            errorElement.style.display = 'block';
            return false;
        }

        for (let i = 0; i < value.length; i++) {
            if (value[i] < '0' || value[i] > '9') {
                errorElement.textContent = '密码只能包含数字';
                errorElement.style.display = 'block';
                return false;
            }
        }

        errorElement.textContent = '';
        errorElement.style.display = 'none';
        return true;
    }

    // 设置转账验证
    function setupTransferValidation() {
        const fromAccount = document.getElementById('transfer_account_id');
        const toAccount = document.getElementById('transfer_to_account');
        const error = document.getElementById('transfer-to-account-error');

        if (fromAccount && toAccount && error) {
            toAccount.addEventListener('input', function () {
                if (fromAccount.value && toAccount.value && fromAccount.value === toAccount.value) {
                    error.textContent = '不能转账给自己';
                    error.style.display = 'block';
                } else {
                    error.style.display = 'none';
                }
            });

            fromAccount.addEventListener('input', function () {
                if (fromAccount.value && toAccount.value && fromAccount.value === toAccount.value) {
                    error.textContent = '不能转账给自己';
                    error.style.display = 'block';
                } else {
                    error.style.display = 'none';
                }
            });
        }
    }

    // 设置表单提交处理
    function setupFormSubmit(operation, formId, resultId, displayCallback) {
        const form = document.getElementById(formId);
        if (form) {
            form.addEventListener('submit', function (e) {
                e.preventDefault();

                // 验证表单
                if (!validateForm(form)) {
                    return;
                }

                const formData = new FormData(form);
                const params = new URLSearchParams();

                for (const [key, value] of formData.entries()) {
                    params.append(key, value);
                }

                // 显示加载提示
                const resultDiv = document.getElementById(resultId);
                resultDiv.className = 'result';
                resultDiv.innerHTML = '<div style="text-align: center;">处理中，请稍候...</div>';

                fetch(`/api/${operation}?${params.toString()}`)
                    .then(response => {
                        if (!response.ok) {
                            throw new Error('服务器响应异常，状态码: ' + response.status);
                        }
                        return response.json();
                    })
                    .then(data => {
                        // 使用回调函数处理结果显示
                        displayCallback(data, form);
                    })
                    .catch(error => {
                        const resultDiv = document.getElementById(resultId);
                        resultDiv.className = 'result error';
                        resultDiv.innerHTML = handleApiError(error, operation);
                    });
            });
        }
    }

    // 转账表单特殊处理
    function setupTransferFormSubmit() {
        const form = document.getElementById('transferForm');
        if (form) {
            form.addEventListener('submit', function (e) {
                e.preventDefault();

                // 验证表单
                if (!validateForm(form)) {
                    return;
                }

                // 检查是否转给自己
                const fromAccount = document.getElementById('transfer_account_id').value;
                const toAccount = document.getElementById('transfer_to_account').value;
                const toName = document.getElementById('transfer_to_name').value;
                const amount = parseFloat(document.getElementById('transfer_amount').value);

                if (fromAccount === toAccount) {
                    document.getElementById('transfer-to-account-error').textContent = '不能转账给自己';
                    document.getElementById('transfer-to-account-error').style.display = 'block';
                    return;
                }

                // 显示确认对话框
                const confirmMsg = `
                  ====== 转账信息确认 ======
                  从己方账号：${fromAccount}
                  转入对方账号：${toAccount}
                  对方姓名：${toName}
                  转账金额：${amount.toFixed(2)}元
                  请仔细核对以上信息！
                `;

                if (!confirm(confirmMsg)) {
                    return;
                }

                // 大额转账二次确认
                if (amount >= 10000) {
                    if (!confirm(`注意：当前为大额转账(${amount.toFixed(2)}元)，是否确认继续？`)) {
                        return;
                    }
                }

                // 准备表单数据
                const formData = new FormData(form);
                const params = new URLSearchParams();

                for (const [key, value] of formData.entries()) {
                    params.append(key, value);
                }

                // 显示加载提示
                const resultDiv = document.getElementById('transferResult');
                resultDiv.className = 'result';
                resultDiv.innerHTML = '<div style="text-align: center;">处理转账请求，请稍候...</div>';

                fetch(`/api/transfer?${params.toString()}`)
                    .then(response => {
                        if (!response.ok) {
                            throw new Error('服务器响应异常，状态码: ' + response.status);
                        }
                        return response.json();
                    })
                    .then(data => {
                        displayTransferResult(data, form);
                    })
                    .catch(error => {
                        resultDiv.className = 'result error';
                        resultDiv.innerHTML = handleApiError(error, 'transfer');
                    });
            });
        }
    }

    // 验证表单
    function validateForm(form) {
        let valid = true;

        // 检查所有必填字段
        Array.from(form.elements).forEach(element => {
            if (element.required && !element.value.trim()) {
                valid = false;
                element.classList.add('invalid');
            } else {
                element.classList.remove('invalid');
            }

            // 检查密码字段
            if (element.name === 'passwd') {
                const errorId = element.id + '-error';
                const errorElement = document.getElementById(errorId) ||
                    document.getElementById(errorId.replace('_', '-'));
                if (errorElement && !validatePassword(element.value, errorElement)) {
                    valid = false;
                }
            }
        });

        return valid;
    }

    // 处理API错误
    function handleApiError(error, operation) {
        let message = `系统错误: ${error.message}`;
        let suggestion = "请稍后再试";

        switch (operation) {
            case 'open':
                suggestion = "请检查您的网络连接并稍后重试";
                break;
            case 'close':
                suggestion = "请确认账户信息是否正确";
                break;
            case 'query':
                suggestion = "请验证您的账号和密码是否正确";
                break;
            case 'transfer':
                suggestion = "请检查账户信息和网络连接";
                break;
        }

        return `<p class="error-message">${message}</p>
                <p class="error-suggestion">${suggestion}</p>`;
    }

    // 开户结果展示
    function displayOpenResult(data, form) {
        const resultDiv = document.getElementById('openResult');
        if (data.result === 0) {
            resultDiv.className = 'result success';
            resultDiv.innerHTML = `
                <h3>开户成功！</h3>
                <p><strong>恭喜，您的新账号是：</strong> 
                   <span class="account-id">${data.message}</span></p>
                <p><strong>初始余额：</strong> 
                   <span class="balance">${parseFloat(data.balance).toFixed(2)}元</span></p>
                <p>请妥善保管您的账号信息</p>
            `;
            // 清空表单
            form.reset();
        } else {
            resultDiv.className = 'result error';
            resultDiv.innerHTML = `<p>开户失败：${data.message}</p>`;
        }
    }

    // 销户结果展示
    function displayCloseResult(data, form) {
        const resultDiv = document.getElementById('closeResult');
        if (data.result === 0) {
            resultDiv.className = 'result success';
            resultDiv.innerHTML = `
                <h3>销户成功</h3>
                <p>您的账户已成功注销</p>
                <p>退还余额: <span class="balance">${parseFloat(data.balance).toFixed(2)}元</span></p>
            `;
            form.reset();
        } else {
            resultDiv.className = 'result error';
            resultDiv.innerHTML = `<p>销户失败：${data.message}</p>`;
        }
    }

    // 存款结果展示
    function displaySaveResult(data, form) {
        const resultDiv = document.getElementById('saveResult');
        if (data.result === 0) {
            const amount = document.getElementById('save_amount').value;
            resultDiv.className = 'result success';
            resultDiv.innerHTML = `
                <h3>存款成功</h3>
                <p>已成功存入：<span class="balance">${parseFloat(amount).toFixed(2)}元</span></p>
                <p>当前账户余额：<span class="balance">${parseFloat(data.balance).toFixed(2)}元</span></p>
                <p>交易时间：${new Date().toLocaleString()}</p>
            `;
            form.reset();
        } else {
            resultDiv.className = 'result error';
            resultDiv.innerHTML = `<p>存款失败：${data.message}</p>`;
        }
    }

    // 取款结果展示
    function displayWithdrawResult(data, form) {
        const resultDiv = document.getElementById('withdrawResult');
        if (data.result === 0) {
            const amount = document.getElementById('withdraw_amount').value;
            resultDiv.className = 'result success';
            resultDiv.innerHTML = `
                <h3>取款成功</h3>
                <p>已成功取出：<span class="balance">${parseFloat(amount).toFixed(2)}元</span></p>
                <p>当前账户余额：<span class="balance">${parseFloat(data.balance).toFixed(2)}元</span></p>
                <p>交易时间：${new Date().toLocaleString()}</p>
            `;
            form.reset();
        } else {
            resultDiv.className = 'result error';
            resultDiv.innerHTML = `<p>取款失败：${data.message}</p>`;
        }
    }

    // 查询结果展示
    function displayQueryResult(data, form) {
        const resultDiv = document.getElementById('queryResult');
        if (data.result === 0) {
            resultDiv.className = 'result success';
            resultDiv.innerHTML = `
                <h3>====== 账户查询结果 ======</h3>
                <table class="result-table">
                    <tr>
                        <td>账号：</td>
                        <td>${document.getElementById('query_account_id').value}</td>
                    </tr>
                    <tr>
                        <td>账户名：</td>
                        <td>${document.getElementById('query_name').value}</td>
                    </tr>
                    <tr>
                        <td>当前余额：</td>
                        <td><span class="balance">${parseFloat(data.balance).toFixed(2)}元</span></td>
                    </tr>
                </table>
                <p>查询时间：${new Date().toLocaleString()}</p>
            `;
        } else {
            resultDiv.className = 'result error';
            resultDiv.innerHTML = `<p>查询失败：${data.message}</p>`;
        }
    }

    // 转账结果展示
    function displayTransferResult(data, form) {
        const resultDiv = document.getElementById('transferResult');
        if (data.result === 0) {
            const amount = document.getElementById('transfer_amount').value;
            const toAccount = document.getElementById('transfer_to_account').value;
            const toName = document.getElementById('transfer_to_name').value;
            resultDiv.className = 'result success';
            resultDiv.innerHTML = `
                <h3>====== 转账成功 ======</h3>
                <p>已成功向账号 ${toAccount}（${toName}） 转账 ${parseFloat(amount).toFixed(2)} 元</p>
                <p><strong>己方账户当前余额：</strong> 
                   <span class="balance">${parseFloat(data.balance).toFixed(2)}元</span></p>
                <p><strong>交易时间：</strong> ${new Date().toLocaleString()}</p>
            `;
            form.reset();
        } else {
            resultDiv.className = 'result error';
            resultDiv.innerHTML = `<p>转账失败：${data.message}</p>`;
        }
    }

    // 设置标签切换
    tabBtns.forEach(btn => {
        btn.addEventListener('click', function () {
            // 移除所有活动状态
            tabBtns.forEach(b => b.classList.remove('active'));
            tabPanes.forEach(p => p.classList.remove('active'));

            // 激活当前标签
            this.classList.add('active');
            const tabId = this.getAttribute('data-tab');
            document.getElementById(tabId).classList.add('active');
        });
    });

    // 初始化表单
    setupForms();
});