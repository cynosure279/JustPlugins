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

## 许可证

GPL-3.0-or-later (与 Phosh 一致)
