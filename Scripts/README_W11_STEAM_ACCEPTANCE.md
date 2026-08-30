# W11 Steam 双机 Release 验收套件

> `W11_PORTABLE_LAUNCH_CONTRACT: RunnerOnlyUserDirOutsidePackage`

只通过 `Tools\RunW11SteamAcceptance.ps1` 启动验收。不要直接双击或运行 `Package\Windows` 下的 EXE 后再把该目录作为原始分发件复制给其他机器：直接启动可能在嵌套 Package 内写入 `Saved/Config`，随后全量清单校验会按设计拒绝这个已污染副本。runner 会把 `UserDir`、日志、角色收据和聚合证据放到套件根级 `Saved/` 或显式套件外目录，并在启动前与真实游戏退出后自动验证套件完整清单和嵌套包，避免依赖操作者记住额外步骤。

本目录可完整复制到另一台 Windows 机器，不需要 Unreal Engine 或项目源码。运行前需要：

- 已登录并拥有项目测试权限的 Steam 客户端；
- Python 3；
- 所有机器使用完全相同的本目录内容、项目 App ID、Scenario 和 EvidenceToken。

每次复制到目标机器后、首次启动前先验证套件：

```powershell
python .\Tools\verify_w11_steam_acceptance_kit.py .\W11SteamAcceptanceKitManifest.json
```

schema 3 套件会拒绝被修改、缺失或额外出现的分发文件；运行时证据只允许写入 `Saved/`。

随后在每台机器运行只读环境预检；它会核对 Python 3、Steam 客户端进程、实际包/套件、清单封存的
runtime EXE，以及 Windows 防火墙中针对该 EXE 的活动允许规则：

```powershell
.\Tools\TestW11SteamAcceptanceEnvironment.ps1 `
  -AppId "<项目 App ID>" `
  -Output ".\Saved\EnvironmentPreflight.json"
```

`Status=READY` 只表示本机启动前置条件齐全，不代表 Steam 联通已经通过。若输出
`ADMIN_REVIEW_REQUIRED / FirewallInspection=ACCESS_DENIED`，或 Windows 安全中心显示“此设置由你的
组织进行管理”且“允许”按钮不可用，应把 JSON 中的精确 `runtimeExecutable` 交给组织管理员按策略
审查；预检不会创建、修改或删除任何防火墙规则。`SteamClient=NOT_RUNNING` 必须先启动并登录 Steam。

Host：

```powershell
.\Tools\RunW11SteamAcceptance.ps1 -Role Host -AppId "<项目 App ID>" `
  -EvidenceToken "W11_<唯一批次>" -Mode Release `
  -PackageManifest ".\Package\W11SteamPackageManifest.json"
```

Client 使用完全相同的命令，只把 `-Role Host` 改为 `-Role Client`。每端成功退出后会同时生成
角色日志和 `*Receipt.json`；收据封存日志 SHA-256、包清单 SHA-256、真实进程目标和退出后整包
复核结果，并记录本机 Steam64 `LocalUserId`。便携套件生成的 schema 2 收据还会封存套件清单
SHA-256、runner-only 外置 UserDir 启动契约，以及启动前/退出后两次全量套件复核结果。集中日志与
对应收据后，使用 `verify_w11_steam_acceptance_logs.py --package-manifest --kit-manifest`
聚合。只有同 Token、同 App ID、同 PackageHash、同 Scenario、同 SessionId、逐日志收据完全匹配，
同一 KitManifestHash，且 Host 真实记录目标参与人数、所有参与者 Steam64 身份互不相同，才能
通过。复制改名的重复日志、重复账号、混用套件或身份与收据不一致都会失败。
Release 启动器直接等待 schema 4 清单封存的真实游戏进程退出；每台机器的单角色退出门禁和
正式多机聚合都必须传 `--package-manifest` 并重新验证实际完整包，单独的 `--package-hash`
不能作为 Cooked 证据来源。

完成 BasicJoin 后，为每个场景使用新的 EvidenceToken 和独立目录重复运行：

- `-Scenario FriendInvite`：Host 建房后从 Steam Overlay 邀请；Client 只能接受邀请加入；
- `-Scenario HostExit`：Client 加入后 Host 退出，Client 必须记录 `NetworkFailure`；
- `-Scenario ClientDisconnect`：Client 加入后关闭，Host 必须记录 `ParticipantLeft`；
- `-Scenario PlayerSync -ExpectedPlayers 2|3|4`：聚合时为每个 Client 重复传入 `--client`。

四人聚合示例：

```powershell
python .\Tools\verify_w11_steam_acceptance_logs.py `
  --host <Host.log> --host-receipt <HostReceipt.json> `
  --client <Client1.log> --client-receipt <Client1Receipt.json> `
  --client <Client2.log> --client-receipt <Client2Receipt.json> `
  --client <Client3.log> --client-receipt <Client3Receipt.json> `
  --scenario PlayerSync --expected-players 4 --app-id <项目 App ID> `
  --token <同一批次 Token> --package-manifest .\Package\W11SteamPackageManifest.json `
  --kit-manifest .\W11SteamAcceptanceKitManifest.json `
  --output .\Saved\Evidence\W11SteamPlayerSync4Evidence.json
```

NAT 网络环境、重连策略和房主迁移仍需单独取证；当前工具不会把这些未证明项自动写成通过。
