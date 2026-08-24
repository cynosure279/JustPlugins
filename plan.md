JustPlugins：Phosh 插件管理入口设计草案

1. 项目目标

为 Phosh 本体增加一个简单、原生的插件管理入口，让用户无需手动操作 GSettings 或寻找 "phosh-mobile-settings" 中隐藏的配置。

第一阶段只管理 Quick Setting 插件。

目标体验：

Phosh 下拉面板

WiFi   蓝牙   旋转
...

[ 管理插件 › ]

点击：

← 插件

JustApps
运行中的后台应用
                    [ON]

Caffeine
防止设备自动休眠
                    [OFF]

Dark Mode
深色模式快捷开关
                    [ON]

开关后立即生效，不需要重启 Phosh。

---

2. 定位

JustPlugins 不是一个 Phosh 插件。

它应当作为 Phosh 本体功能存在。

原因：

插件管理器也是插件
      ↓
用户关闭插件管理器
      ↓
无法重新开启插件

因此架构应为：

Phosh
 ├── Quick Settings
 ├── Plugin Loader
 └── JustPlugins / Plugin Manager UI

项目最终如果适合上游，可以整理成 Phosh merge request。

---

3. 现有 Phosh 机制

不要重新设计插件系统。

Phosh 已经存在：

*.plugin metadata
      ↓
PhoshPluginLoader
      ↓
GIO extension point
      ↓
Quick Setting instance

启用列表由：

sm.puri.phosh.plugins
    quick-settings

保存。

例如：

[
  'justapps',
  'caffeine-quick-setting'
]

Phosh 当前已经监听该 GSettings key。

修改列表后：

GSettings changed
      ↓
PhoshQuickSettings
      ↓
重新加载 custom quick settings

因此插件开关可以实时生效。

---

4. 插件发现

不要硬编码插件名单。

扫描 Phosh 插件目录中的：

*.plugin

例如：

/usr/lib/phosh/plugins/

从 metadata 中读取：

[Plugin]
Id=justapps
Name=JustApps
Comment=Show running tray applications
Types=quick-setting;
Icon=justapps-symbolic
Plugin=libphosh-plugin-justapps.so

第一版只接受：

Types 包含 quick-setting

每个插件至少获得：

id
name
comment
icon
enabled

其中：

enabled =
    id 是否存在于
    sm.puri.phosh.plugins quick-settings

---

5. UI 设计

建议增加一个：

PhoshPluginManagerPage

使用普通 Phosh/GTK Widget。

页面：

Plugins

┌──────────────────────────┐
│ [icon] JustApps      [●] │
│ Running tray apps        │
├──────────────────────────┤
│ [icon] Caffeine      [○] │
│ Prevent automatic idle   │
├──────────────────────────┤
│ [icon] Dark Mode     [●] │
│ Dark mode quick setting  │
└──────────────────────────┘

要求：

- 手机触屏友好
- 每行整个区域足够大
- 名称 + Comment
- 插件图标存在则显示
- Switch 显示启用状态

插件 metadata 损坏时：

- 不 crash
- 跳过或显示 Unknown Plugin
- 输出 warning

---

6. 管理入口

第一版选择最简单的入口即可。

推荐：

Phosh Quick Settings drawer
          ↓
      管理插件

它本身不是 custom Quick Setting plugin，而是 Phosh 内建入口。

可以放在：

- Quick Settings 编辑/管理区域
- 或 Quick Settings 页面底部

不要为了第一版调整整个下拉面板布局。

目标只是：

Manage Plugins ›

能够进入 "PhoshPluginManagerPage"。

---

7. 开关行为

开启：

old:
['caffeine-quick-setting']

toggle JustApps ON

new:
[
 'caffeine-quick-setting',
 'justapps'
]

关闭：

old:
[
 'caffeine-quick-setting',
 'justapps'
]

toggle JustApps OFF

new:
['caffeine-quick-setting']

必须保留其他插件。

禁止使用会覆盖整个列表的错误逻辑，例如：

set ['justapps']

---

8. 排序

第一版不做 UI 排序。

但代码设计不要破坏现有顺序。

开启新插件时默认追加：

[A, B]
   +
[C]

→ [A, B, C]

关闭时只删除目标：

[A, B, C]
关闭 B
→ [A, C]

第二阶段再实现拖拽或上下移动：

JustApps      ↑ ↓
Caffeine      ↑ ↓
Dark Mode     ↑ ↓

排序本质上直接修改 GSettings string array 顺序。

---

9. Preferences

第一版不在 Phosh 内嵌插件 Preferences。

如果 ".plugin" 包含：

[Prefs]
...

第一阶段可以忽略。

第二阶段可增加：

JustApps       [ON]  ⚙

点击 ⚙ 后调用现有 "phosh-mobile-settings" 打开对应插件设置。

不要把 libadwaita prefs UI 直接嵌进 Phosh shell。

---

10. 建议代码结构

示意：

src/
├── plugin-manager.c
├── plugin-manager.h
├── plugin-manager-page.c
├── plugin-manager-page.h
├── plugin-info.c
├── plugin-info.h
└── ui/
    └── plugin-manager-page.ui

职责：

PluginInfo
    │
    ├── 解析 *.plugin
    └── 保存 metadata

PluginManager
    │
    ├── 扫描插件
    ├── 读取 GSettings
    ├── enable()
    └── disable()

PluginManagerPage
    │
    └── UI

不要把扫描、GSettings 修改和 GTK UI 全写在一个文件里。

---

11. PluginInfo 建议字段

typedef struct {
    char *id;
    char *name;
    char *comment;
    char *icon;
    char *module;
    gboolean enabled;
    gboolean has_prefs;
} PhoshPluginInfo;

后续可扩展：

types
version
author
prefs id

第一版无需使用。

---

12. 安全与稳定性

Phosh 是 shell，插件管理页面不能因为坏 metadata 把整个桌面炸掉。

要求：

- ".plugin" 缺字段时安全处理
- 重复 Id 处理
- 无效 UTF-8 处理
- 插件目录不存在时正常显示空页面
- GSettings 写入失败时恢复 Switch 状态
- 插件加载失败时不能导致 Plugin Manager crash
- 不直接 dlopen 每一个插件来获取信息
- 插件枚举只读 metadata

特别注意：

发现插件 ≠ 加载插件

Plugin Manager 不应该为了显示列表而实例化所有插件。

---

13. 第一阶段开发顺序

Phase 1：读取当前状态

实现：

读取 quick-settings GSettings

正确得到当前启用插件列表。

Phase 2：扫描 metadata

扫描 "*.plugin"。

输出：

Id
Name
Comment
Icon
enabled

此阶段甚至可以先只打印日志。

Phase 3：Plugin Manager Page

做出页面：

JustApps       OFF
Caffeine       ON

暂时 Switch 不操作。

Phase 4：启停

连接 Switch：

toggle
 ↓
修改 GSettings
 ↓
Phosh 自动热加载/卸载插件

验证真实插件能够实时消失/出现。

Phase 5：集成入口

将：

Manage Plugins

加入 Phosh 下拉管理区域。

---

14. MVP 验收标准

第一版只要求：

1. 能自动发现已安装的 Quick Setting 插件。
2. 不硬编码插件名单。
3. 正确读取名称、描述和图标。
4. 正确判断插件启用状态。
5. 能开启插件。
6. 能关闭插件。
7. 不影响其他插件。
8. 修改后无需重启 Phosh。
9. 插件 metadata 有问题时 Phosh 不 crash。
10. JustApps 能通过该页面正常开启和关闭。

---

15. 暂时不要做

MVP 阶段不要加入：

- Lock Screen plugin 管理
- Status Icon plugin 管理
- 插件商店
- 在线下载安装插件
- 插件更新
- 权限系统
- 搜索
- 分类
- 完整 Preferences renderer
- 大规模重构 "PhoshPluginLoader"

先证明：

发现 → 展示 → 开关 → 热加载

这一条链完整工作。

项目暂定名称 JustPlugins；内部类名建议仍使用通用的 "PhoshPluginManager*"，这样以后如果提交 Phosh 上游，不需要再进行大量命名重构。