# mysql-fast-importer 🚀
A fast MySQL SQL file importer with Windows GUI, bypassing Navicat limits using mysql.exe.
一个基于 C 语言与原生 Win32 API 开发的 Windows 数据库高速导入工具。通过直接调用系统底层的 `mysql.exe`，完美绕过 Navicat 等 GUI 工具的导入大文件限制、卡死与速度瓶颈。

---

## ✨ 核心特性

- **极致速度**：不经过复杂的图形客户端解析，直接利用 MySQL 官方原生命令行重定向，导入速度比传统 GUI 工具快数倍。
- **超强稳定性**：完美支持 GB 级别的超大 `.sql` 文件导入。
- **纯净无依赖**：纯 C 语言配合原生 Win32 API 编写，无庞大框架，绿色免安装。
- **防卡死 UI**：核心导入逻辑运行于独立的后台异步线程，大文件导入时界面依旧能流畅拖动，支持实时捕获并弹窗提示详细的报错日志。
- **极致体验**：支持 Windows 窗口文件拖拽（Drag & Drop），直接拖入 `.sql` 文件即可自动识别路径（规划中）。

---

## 🛠️ 技术构想与架构

程序通过捕获用户的界面输入，在后台通过 `CreateProcess` 异步调用并拼接为如下底层命令：
```bash
cmd.exe /c "mysql -u <用户名> -p<密码> < "<文件路径>""
```
利用 **Windows 匿名管道（Pipe）** 技术，实时重定向并捕获 `stderr` 错误流，确保在导入失败时能精准将 MySQL 官方报错弹窗反馈给用户。

---

## 🚀 快速开始

### 1. 前提条件
- 操作系统：Windows 7 或更高版本。
- 目标电脑已配置 MySQL 环境变量（或确保 `mysql.exe` 可全局调用）。

### 2. 使用方法
1. 下载并运行 `mysql-fast-importer.exe`。
2. 输入您的数据库**用户名**和**密码**。
3. 点击“浏览”或直接拖入您需要导入的 `.sql` 文件。
4. 点击 **“开始导入”** 按钮。
5. 导入完成后，程序会自动弹出“成功”或带详细日志的“失败”提示窗口。

---

## 📅 路线图 (Roadmap)

- [x] 基于原生 Win32 API 的初始 UI 界面搭建
- [ ] 集成 Windows 原生文件选择器 (`GetOpenFileName`)
- [ ] 开启 `WM_DROPFILES` 实现拖拽文件自动填入路径
- [ ] 基于 `CreateThread` 的后台异步导入引擎与管道错误捕获
- [ ] 自动检测/手动指定本地 `mysql.exe` 路径功能

---

## 📄 开源协议

本项目基于 [MIT License](LICENSE) 协议开源。
