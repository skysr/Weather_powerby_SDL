# Weather_powerby_SDL

基于 SDL3 的实时天气仪表板应用，展示广州实时天气数据、模拟时钟、月历和节假日信息。

## 功能特性

- **实时天气数据**：温度、湿度、风速、风向、气压
- **空气质量指数**：AQI 实时监测
- **日出日落时间**：根据地理位置自动计算
- **模拟时钟**：实时显示时分秒针
- **月历显示**：完整月历网格，高亮当前日期
- **节假日提醒**：显示下一个中国法定节假日
- **二十四节气**：显示当前/最近节气
- **Bing 每日壁纸**：自动获取 Bing 壁纸作为背景（sdl02.c）

## 文件说明

| 文件 | 描述 |
|------|------|
| `sdl01.c` | 基础版本，纯色渐变背景 |
| `sdl02.c` | 增强版本，Bing 每日壁纸背景 |

## 依赖库

- **SDL3** - 窗口、渲染、事件处理
- **SDL3_ttf** - TrueType 字体渲染
- **SDL3_image** - 图像加载（sdl02.c 需要用于壁纸）
- **libcurl** - HTTP 客户端
- **json-c** - JSON 解析
- **math** - 数学运算（时钟绘制）

## 编译方法

### sdl01.c（基础版）

```bash
gcc -o sdl-01 sdl01.c \
    $(pkg-config --cflags --libs sdl3 sdl3-ttf) \
    -lcurl -ljson-c -lm
```

### sdl02.c（壁纸版）

```bash
gcc -o sdl-02 sdl02.c \
    $(pkg-config --cflags --libs sdl3 sdl3-ttf sdl3-image) \
    -lcurl -ljson-c -lm
```

## 运行

```bash
./sdl-01   # 基础版
./sdl-02   # 壁纸版
```

按 `ESC` 键退出应用。

## 数据来源

- **天气数据**：[Open-Meteo API](https://open-meteo.com/) - 免费，无需 API Key
- **空气质量**：[WAQI API](https://waqi.info/) - 使用 demo token
- **背景壁纸**：[Bing HPImageArchive](https://www.bing.com/HPImageArchive.aspx) - 每 5 分钟更新

## 代码注释

两个源文件都添加了详细的中文注释：

### 文件结构注释
- 文件头部：功能描述、依赖列表、编译命令、运行方法
- 模块分隔：使用 `/* === */` 分隔不同功能模块

### 全局变量注释
- 颜色常量：每个颜色的用途说明
- 数据变量：存储内容的描述

### 函数注释
每个函数包含：
- `@brief` - 简要描述
- `@param` - 参数说明
- `@return` - 返回值说明
- `@note` - 特殊注意事项

### 结构体注释
- `SolarTerm` - 二十四节气信息，包含每个节气的含义说明

### API 文档
- Open-Meteo API 调用参数说明
- WAQI API AQI 分级标准
- Bing API 工作流程说明

## 界面布局

```
┌─────────────────────────────┬──────────┐
│  温度        天气图标        │          │
│  24°C        ☀☁🌧          │          │
│  Guangzhou/CN               │  月历    │
│                             │  April   │
│        ┌─────┐              │  25 Fri │
│        │ 时钟 │              │  S M T ...│
│        │ ○   │              │  1 2 3...│
│        └─────┘              │          │
│                             │  节假日  │
│  湿度 风速 AQI 气压 日出    │          │
└─────────────────────────────┴──────────┘
```

## 许可证

MIT License
