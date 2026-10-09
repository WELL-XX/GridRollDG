# GridRollDG

PCB1 专用 ZMK 分体键盘固件：右侧主机、左侧无线从机，47 键、左侧 EC11 旋钮、右侧 PAW3222 轨迹球，支持官方 ZMK Studio 经右侧 USB 修改键位。

本仓库对应嘉立创EDA的「分体 plank PCB v1.1 / PCB1」，主控为 E73-2G4M08S1CX。它与 WELL-XX/GridRoll 使用不同矩阵、ADC 和板级配置；只参考该仓库的轨迹球通信与引导程序框架。来源及许可证见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 下载与烧录

在本仓库 **Actions → Build GridRollDG firmware** 中打开最新成功的运行，下载 **GridRollDG-ready-to-flash**。

| 文件 | 用途 |
| --- | --- |
| `GridRollDG-left-first-flash.hex` | 左侧首次 SWD 烧录，含 MBR、SoftDevice、引导程序、应用及 UICR |
| `GridRollDG-right-first-flash.hex` | 右侧首次 SWD 烧录，含完整启动链及应用 |
| `GridRollDG-left.uf2` / `GridRollDG-right.uf2` | 已装引导程序后的日常 USB 更新 |
| `GridRollDG-right-diagnostics.uf2` | 右侧 RTT 日志诊断版本 |
| `GridRollDG-settings-reset.uf2` | 两侧分别清除旧蓝牙配对和保存的键位；随后重新刷对应侧固件 |
| `GridRollDG-first-flash.hex` | 通用引导程序底包，没有键盘应用，通常无需单独使用 |
| `SHA256SUMS` / `version.json` | 文件校验值及依赖、提交版本 |

首次使用需要 SWD 烧录器。完整步骤见 [首次烧录](docs/first-flash.md)，日常配对及键位修改见 [使用说明](docs/usage.md)。

## 默认布局

```text
Esc    Q     W     E     R     T     | Y      U     I       O     P      Backspace
Tab    A     S     D     F     G     | H      J     K       L     ;      Enter
Shift  Z     X     C     V     B     | N      M     ,       .     /      Shift
Ctrl   GUI   Alt   静音  Fn    Space | Mouse  Left  [轨迹球] Down  Up     Right
```

Fn 为数字、F1–F12、符号和导航层；Mouse 为鼠标按钮及轨迹球滚动层。Fn + Mouse 同时按住进入系统层。旋钮正常调音量，按压静音；Mouse 层每个卡点发出一次滚轮事件。

## 构建与检查

提交、拉取请求和手动运行都会构建左侧、右侧、右侧诊断和设置清除固件，以及专用 Adafruit UF2 引导程序。Actions 自动检查 PCB1 GPIO、47 键映射、NFC/复位配置、ADC 分压、旋钮参数、SDIO 错误恢复、UF2/HEX 内容一致和存储区间，再打包左右侧首次烧录文件。

核心依赖固定为官方 ZMK v0.3.0、其 Zephyr 3.5 分支的指定提交，以及 Adafruit nRF52 Bootloader 0.11.0。源码、构建日志、配置和校验值随产物保存。详细配置见 [硬件依据](docs/hardware.md)。

**实物验收尚需上板完成。** 编译通过、网络表检查与实际按键、旋钮、轨迹球、蓝牙、Studio、电池读数测试分别记录在 [验收记录](docs/validation.md)。
