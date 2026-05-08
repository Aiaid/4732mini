# 4732mini

基于 **SI4732 / SI4735** 的全波段(FM / MW / SW / LW / SSB)收音机固件,运行于 **Lilygo T-Display-S3** (ESP32-S3 + 320×170 ST7789)。

## 致谢与衍生关系

本项目是以下开源工作的衍生作品:

- **[PU2CLR / SI4735 Arduino Library](https://github.com/pu2clr/SI4735)** —— Ricardo Lima Caratti, MIT License。本仓库的核心收音机逻辑 (Band 表、SSB patch 加载、菜单状态机) 改编自该库 `examples/SI47XX_KITS/Lilygo_t_embed/` 下的 `ALL_IN_ONE_T_Embed` 示例。
- **[VolosR / TEmbedFMRadio](https://github.com/VolosR/TEmbedFMRadio)** —— 原始 UI 设计,后由 PU2CLR 移植到 SI473x。
- **[zhang-chong / 4732mini](https://github.com/zhang-chong/4732mini)** —— T-Display-S3 端的移植,中文菜单、电台预设、自动熄屏、电池电压校准等。

本仓库 (`Aiaid/4732mini`) 是上一项的 fork,沿继续维护与改进。

## 协议

本项目以及所衍生的所有上游代码均遵循 **MIT License**,详见 [`LICENSE`](./LICENSE) 文件。MIT 协议允许商业使用、修改和再分发,唯一硬性要求是在副本中保留版权声明与协议文本。

> 原始作者建议"仅供学习交流使用,请勿用作商业用途"。这是道德请求,法律上 MIT 协议本身不施加此限制 —— 但请尊重上游作者的意愿。如果改进了代码,欢迎提 PR 回馈社区。

## 构建 / 烧录

使用 Arduino IDE:

- 板子: ESP32S3 Dev Module 或 LilyGo T-Display-S3
- USB CDC On Boot: Enabled
- Flash 16MB,PSRAM OPI
- **必须**使用本仓库 `libraries/` 下的库副本(尤其是定制过的 `TFT_eSPI_DynamicSpeed`,引脚映射已为 T-Display-S3 配置好);把 `libraries/` 拷贝或软链到 `~/Documents/Arduino/libraries/`。

打开 `ALL_IN_ONE_T-Display_S3.ino`,选好板子后直接烧录。
