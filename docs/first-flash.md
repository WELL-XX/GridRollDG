# 首次 SWD 烧录与 UF2 更新

## 第一次烧录

1. 下载最新成功 Actions 的 `GridRollDG-ready-to-flash` 并解压。确认 `version.json` 对应所需提交；可用 `SHA256SUMS` 校验文件。
2. 准备支持 nRF52840 的 SWD 烧录器，例如 J-Link，以及 nRF Connect for Desktop Programmer 或烧录器配套工具。
3. 对应侧接 GND、SWDIO、SWDCLK 和 3.3 V 电压参考。E73 的 SWDIO 为模块焊盘 37，SWDCLK 为 39；这些是模块焊盘编号。PCB 正常由其 3.3 V 电源供电，烧录器的电压参考按工具说明连接，避免两路供电相互驱动。
4. 选择芯片 nRF52840。全新模块执行一次整片擦除，再烧录：左侧 `GridRollDG-left-first-flash.hex`，右侧 `GridRollDG-right-first-flash.hex`。
5. 启用烧录后的校验，复位或断电再上电。完整 HEX 已包含应用和全部启动链，不需要额外刷 SoftDevice。
6. 将右侧 USB 连接电脑，先测试右侧按键；再给左侧上电，测试左右连接。配对详见 [使用说明](usage.md)。

整片擦除会清除旧应用、键位和配对；首次烧录后日常使用 UF2 更新即可。此包使用内部低频 RC，无需外接低频晶体。

## 日常更新

1. 给需要更新的一侧连接支持数据传输的 USB 线。
2. 快速双击该侧复位按钮（P0.18），等待出现 `GRIDDGBOOT` 磁盘。也可通过系统层的引导程序按键进入。
3. 左侧复制 `GridRollDG-left.uf2`，右侧复制 `GridRollDG-right.uf2` 到该磁盘。写入完成后自动重启，磁盘消失属于正常现象。
4. 重新测试连接和键位。左右文件不可混用，两侧磁盘名称相同，应逐侧更新。

右侧诊断文件可代替右侧普通 UF2，用 SWD 的 RTT 工具读取日志；测试后刷回正常右侧版本。

## 清除配对或恢复默认键位

当更换 ZMK 分体固件、无法重新建立左右连接，或需要清除 Studio 保存配置时：在每一侧分别刷 `GridRollDG-settings-reset.uf2`，等待清除完成，再进入引导程序并刷回该侧正常 UF2。随后删除电脑中旧的 GridRollDG 蓝牙记录，重新配对。此过程会清除保存的键位与配对信息。

如果无法出现 UF2 磁盘，先检查 USB 数据线、对应侧供电和复位按钮；尚未完成首次 SWD 烧录的模块不能直接使用 UF2。详细排查见 [使用说明](usage.md)。
