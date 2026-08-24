# JustPlugins — Phosh Plugin Manager

为 Phosh 下拉面板提供原生插件管理入口。**本项目是提交到 Phosh 上游的 PR。**

## 项目定位

JustPlugins **不是**一个 Phosh 插件。它是 Phosh 本体的原生功能，位于 Quick Settings 面板中，提供插件开关管理。

## 架构

```
Phosh 本体
 ├── Quick Settings 面板
 │    ├── 默认开关 (WiFi, BT, 旋转, 手电筒...)
 │    ├── 自定义插件 (GIO extension point 热加载)
 │    └── [ 管理插件 › ]  ← 内置入口
 │
 ├── Plugin Loader (g_io_modules_scan_all_in_directory)
 │
 └── PhoshPluginManager (本 PR 新增)
      ├── PluginInfo      — 解析 *.plugin 元数据文件
      ├── PluginManager   — 扫描插件目录 + 读写 GSettings
      └── PluginManagerPage — GTK 列表页面 + Switch 开关
```

## 文件列表

### 新文件（提交到 PR）

| 文件 | 行数 | 职责 |
|------|------|------|
| `src/plugin-info.c` | 87 | 解析 .plugin keyfile 元数据 |
| `src/plugin-info.h` | 55 | PhoshPluginInfo 结构体定义 |
| `src/plugin-manager.c` | 215 | 扫描插件目录、GSettings 读写 |
| `src/plugin-manager.h` | 63 | PhoshPluginManager 接口 |
| `src/plugin-manager-page.c` | 276 | 插件管理 UI 页面 |
| `src/plugin-manager-page.h` | 28 | PhoshPluginManagerPage 接口 |
| `src/ui/plugin-manager-page.ui` | 109 | 页面模板 (返回按钮 + 列表 + 空状态) |

### 修改的 Phosh 文件

| 文件 | 修改内容 |
|------|----------|
| `src/quick-settings.c` | 集成 PluginManager，添加 "管理插件" 入口和 back-clicked 信号 |
| `src/ui/quick-settings.ui` | 添加 GtkStack 容器和 "管理插件" 按钮 |
| `src/phosh.gresources.xml` | 添加 plugin-manager-page.ui 资源 |
| `src/meson.build` | 添加新源文件到 phosh_tool_sources 和 phosh_headers |

## 实现细节

### 插件发现

```c
// 扫描 /usr/lib/phosh/plugins/ 目录下的 *.plugin 文件
// 只处理 Types 包含 quick-setting 的插件
// 损坏的 metadata 跳过不 crash
```

### 启用/禁用

```c
// 读取 GSettings sm.puri.phosh.plugins quick-settings
// 开启 → append 到列表末尾（不破坏现有顺序）
// 关闭 → 只从列表中移除目标
// GSettings 写入失败 → revert Switch 状态
```

### 热加载

利用 Phosh 现有的 `quick-settings.c` 中 `load_custom_quick_settings()` 的 GSettings 监听机制：
`g_signal_connect (plugin_settings, "changed::quick-settings", ...)`

修改后无需重启 Phosh，插件立即生效。

## 构建

```bash
# 克隆 Phosh 源码
git clone https://gitlab.gnome.org/World/Phosh/phosh.git
cd phosh

# 复制 JustPlugins 新文件
cp /path/to/JustPlugins/src/plugin-info.c src/
cp /path/to/JustPlugins/src/plugin-info.h src/
cp /path/to/JustPlugins/src/plugin-manager.c src/
cp /path/to/JustPlugins/src/plugin-manager.h src/
cp /path/to/JustPlugins/src/plugin-manager-page.c src/
cp /path/to/JustPlugins/src/plugin-manager-page.h src/
cp /path/to/JustPlugins/src/ui/plugin-manager-page.ui src/ui/

# 覆盖修改的文件
cp /path/to/JustPlugins/src/quick-settings.c src/
cp /path/to/JustPlugins/src/ui/quick-settings.ui src/ui/
cp /path/to/JustPlugins/src/phosh.gresources.xml src/
cp /path/to/JustPlugins/src/meson.build src/

# 构建
meson setup _build -Dtools=true -Dtests=false
meson compile -C _build
```

## CI/CD

GitHub Actions 自动构建：
1. 安装 Phosh 构建依赖
2. 克隆 Phosh 最新源码
3. 应用 JustPlugins 修改
4. Meson 构建
5. 上传构建产物

### 多平台 CI

| 平台 | 容器 | 状态 |
|------|------|------|
| Debian Trixie | `debian:trixie` | GNOME 47+ 完整构建 |
| Alpine Edge | `alpine:edge` | postmarketOS 基础环境 |

### 发布流程

1. 打 tag：`git tag v0.1.0 && git push origin v0.1.0`
2. Release workflow 自动生成 overlay tarball 并创建 GitHub Release
3. 下载 APKBUILD + overlay tarball，在 Alpine/pmOS 上 `abuild -r` 构建

## 在 postmarketOS / Alpine 上安装

### 方式一：APKBUILD 构建（推荐）

```bash
# 克隆仓库
 git clone https://github.com/cynosure279/JustPlugins.git
cd JustPlugins/apkbuild

# 方法 A：使用 GitHub Release 的 overlay tarball
# 确保 APKBUILD 中的 _overlayver 和 sha512sums 与 Release 版本匹配
abuild -r

# 方法 B：本地生成 overlay tarball
cd ..
bash scripts/make-overlay.sh 0.1.0
# 将生成的 dist/justplugins-overlay-0.1.0.tar.gz 放到 ~/packages/ 下
cd apkbuild
abuild -r

# 安装
sudo apk add --allow-untrusted ../$(uname -m)/phosh-justplugins-*.apk

# 重启 Phosh
sudo rc-service phosh restart  # OpenRC
# 或
systemctl --user restart phosh  # systemd
```

### 方式二：pmbootstrap（postmarketOS 开发者）

```bash
# 将 apkbuild/ 作为本地 pmaports overlay
pmbootstrap init
# 选择你的设备，然后在 pmaports 中添加本地包

# 或手动构建
pmbootstrap build phosh-justplugins
pmbootstrap install phosh-justplugins
```

### APKBUILD 说明

| 字段 | 值 | 说明 |
|------|-----|------|
| `pkgname` | `phosh-justplugins` | 替换 phosh 的 overlay 包 |
| `provides` | `phosh=$pkgver` | 满足 phosh 依赖 |
| `conflicts` | `phosh` | 不能与原版共存 |
| `source` | phosh tarball + overlay tarball | 双源构建 |
| `makedepends` | 与 Alpine phosh 一致 | 使用 Alpine 包名 |

## 文件结构

```
JustPlugins/
├── src/                          # 源码（提交到 Phosh PR）
│   ├── plugin-info.c/h           # 插件元数据解析
│   ├── plugin-manager.c/h         # 插件管理器（GSettings 读写）
│   ├── plugin-manager-page.c/h    # UI 页面
│   ├── quick-settings.c           # 修改的 Phosh 文件
│   ├── ui/                        # GTK 模板
│   ├── meson.build               # 修改的构建文件
│   └── phosh.gresources.xml      # 修改的资源文件
├── apkbuild/
│   └── APKBUILD                  # Alpine/postmarketOS 打包文件
├── scripts/
│   └── make-overlay.sh           # overlay tarball 生成脚本
├── .github/workflows/
│   ├── build.yml                 # CI 构建 (Debian + Alpine)
│   └── release.yml               # 发布 (tag 触发)
└── plan.md                       # 设计文档
```

## 许可证

GPL-3.0-or-later (与 Phosh 一致)
