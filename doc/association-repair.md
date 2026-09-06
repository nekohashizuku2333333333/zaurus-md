# 卸载后 IPK 关联丢失的修复

## 原因

0.1 包的 `data.tar.gz` 含有系统文件
`/home/QtPalmtop/apps/Settings/qinstall.desktop`。
旧 ipkg 将归档内文件全部记入 `zaurusmd.list`，卸载时逐个删除。
因此即使安装时恢复了该入口，卸载仍然会把它删掉。
更早的包还直接登记过两个 `mime.types` 文件，存在同样的问题。

## 0.2 的处理

- 包内只包含编辑器自身的可执行文件、桌面入口、恢复工具和许可证。
- 原生 `qinstall.desktop` 和系统 MIME 文件不再属于编辑器包。
- `postinst` 恢复 `.ipk -> application/ipkg -> qinstall`，按需补充 Markdown。
- `prerm` 仅删除带本包标记的 Markdown 注册。系统关联保留。
- MIME 文件存在时保留现有其他映射，只纠正 `ipk` 扩展的归属；文件缺失或为空时
  才使用完整 Qtopia 基线，避免创建仅含 IPK 的残缺表。
- 原生入口按 rootfs 保存的 Sharp 配置恢复，停用此前误引入的 `qipkg.desktop`。
- 已存在的 `slmime.types` 分类不覆盖；缺失时恢复原始基线。
- 被修改的现有文件第一次备份为 `.zaurusmd-before-ipk-fix`。
- 不写入 symlink 指向的 ROM 文件，修复结果放在可写的覆盖层路径。
- 版本号从 0.1 提升至 0.2，允许直接升级。

## 恢复已卸载的设备

使用 `DIST/restore-ipk-association.sh`。它包含所需原始配置，不依赖编辑器已安装，
也不依赖 `.ipk` 文件关联。在真机终端以 root 执行：

```sh
sh restore-ipk-association.sh
```

也可以直接在终端使用 `ipkg install zaurusmd_0.2_arm.ipk`，由新包自动恢复关联。
脚本会尝试通知 Qtopia 刷新入口；文件管理器缓存未更新时，保存工作后重新打开
文件管理器或重启 Qtopia，不会自动结束正在编辑的应用。

脚本不重建丢失的 `qinstall` 可执行文件；该二进制从未属于本项目 IPK。
如果它确实缺失，脚本会报告，需要另按对应 rootfs 恢复二进制。

## 已执行验证

- 51 项本地隔离检查通过：缺失文件恢复、保留 TXT 和自定义类型、脚本幂等、
  安装/卸载/重装、仅移除自有 Markdown 项、ROM symlink 目标不变。
- 使用 SDK 主机的旧 ipkg、隔离配置及禁用的外部链接辅助程序，实际复现了
  旧包卸载删除 `qinstall.desktop`。
- 独立恢复、0.2 安装后卸载、重新安装后卸载，以及从旧包直接升级后卸载均通过。
- 测试没有操作真机。已知 SSH 地址 `192.168.122.187` 是构建机，
  不含 `/home/QtPalmtop`；真机需要执行上述脚本或提供其连接信息。
