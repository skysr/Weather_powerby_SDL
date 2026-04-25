/**
 * @file sdl01.c
 * @brief SDL3 天气仪表板应用 - 基础版本
 *
 * 这是一个基于 SDL3 的实时天气仪表板应用，显示：
 * - 广州实时天气数据（温度、湿度、风速、气压）
 * - 空气质量指数 (AQI)
 * - 日出日落时间
 * - 模拟时钟
 * - 月历和节假日信息
 * - 二十四节气
 *
 * @details 数据来源：
 * - 天气数据：Open-Meteo API (https://open-meteo.com)
 * - 空气质量：WAQI API (https://waqi.info)
 * - 节假日：本地计算
 *
 * @dependencies 依赖库：
 * - SDL3 (Simple DirectMedia Layer 3.0+)
 * - SDL3_ttf (TrueType 字体渲染)
 * - libcurl (HTTP 客户端)
 * - json-c (JSON 解析)
 * - math (数学运算)
 *
 * @compile 编译方法 (Linux):
 * @code
 * gcc sdl01.c -o weather_dashboard \
 *     $(pkg-config --cflags --libs sdl3 sdl3-ttf) \
 *     -lcurl -ljson-c -lm
 * @endcode
 *
 * @usage 运行：
 * @code
 * ./weather_dashboard
 * 按 ESC 键退出
 * @endcode
 *
 * @author skysr
 * @version 1.0
 * @date 2025
 * @see https://github.com/skysr/Weather_powerby_SDL
 */

/* ============================================================================
 * 头文件包含
 * ============================================================================ */

/* SDL3 回调模式：使用 SDL_AppInit/SDL_AppIterate/SDL_AppQuit 替代 main */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>           /* SDL 核心库：窗口、渲染、事件 */
#include <SDL3_ttf/SDL_ttf.h>    /* SDL_ttf：TrueType 字体渲染 */
#include <SDL3/SDL_main.h>       /* SDL main 宏：回调模式入口 */

#include <time.h>                /* 时间处理：获取当前时间、格式化 */
#include <stdio.h>              /* 标准输入输出：printf、snprintf */
#include <string.h>             /* 字符串处理：strlen、memcpy */
#include <math.h>                /* 数学运算：sin、cos、M_PI */
#include <stdlib.h>             /* 标准库：malloc、free、realloc */
#include <curl/curl.h>          /* libcurl：HTTP 客户端 */
#include <json-c/json.h>        /* json-c：JSON 解析 */

/* ============================================================================
 * 屏幕尺寸配置
 * ============================================================================ */

#define SCREEN_WIDTH 1280        /* 窗口宽度（像素） */
#define SCREEN_HEIGHT 800       /* 窗口高度（像素） */

/* ============================================================================
 * 颜色定义
 * 所有颜色使用 RGBA 格式（红、绿、蓝、透明度）
 * ============================================================================ */

/* 基础颜色 - 用于文字和图标 */
static const SDL_Color COLOR_WHITE = {255, 255, 255, 255};   /* 纯白色：主要文字 */
static const SDL_Color COLOR_GRAY = {180, 180, 180, 255};    /* 灰色：次要文字 */
static const SDL_Color COLOR_CYAN = {100, 200, 255, 255};    /* 青色：强调信息 */
static const SDL_Color COLOR_PURPLE = {180, 100, 255, 255};  /* 紫色：周末日期 */
static const SDL_Color COLOR_ORANGE = {255, 150, 50, 255};   /* 橙色：当前日期 */
static const SDL_Color COLOR_BLUE = {100, 150, 255, 255};    /* 蓝色：一般信息 */

/* 面板和卡片颜色 - 半透明背景 */
static const SDL_Color COLOR_PANEL_BG = {30, 35, 60, 230};    /* 主面板背景：深蓝紫 */
static const SDL_Color COLOR_CARD_1 = {30, 50, 90, 200};      /* 卡片1：深蓝 */
static const SDL_Color COLOR_CARD_2 = {30, 60, 100, 200};     /* 卡片2：中蓝 */
static const SDL_Color COLOR_CARD_3 = {40, 70, 110, 200};     /* 卡片3：浅蓝 */
static const SDL_Color COLOR_CARD_4 = {60, 50, 120, 200};     /* 卡片4：紫蓝 */
static const SDL_Color COLOR_CARD_5 = {100, 50, 80, 200};     /* 卡片5：玫红 */

/* ============================================================================
 * SDL 全局变量
 * ============================================================================ */

static SDL_Window *window = NULL;      /* 窗口句柄 */
static SDL_Renderer *renderer = NULL;  /* 渲染器句柄 */
static TTF_Font *font_large = NULL;   /* 大字体：72px，用于温度显示 */
static TTF_Font *font_medium = NULL;  /* 中字体：32px，用于日期、区域 */
static TTF_Font *font_small = NULL;   /* 小字体：18px，用于标签、数字 */

/* ============================================================================
 * 天气数据存储
 * 所有数据以字符串形式存储，便于直接渲染
 * ============================================================================ */

static char g_tempStr[32] = "24°C";          /* 温度：如 "24°C" */
static char g_humidityStr[32] = "65%";       /* 湿度百分比 */
static char g_humidityDesc[32] = "Comfortable";  /* 湿度描述：Dry/Comfortable/Humid */
static char g_windStr[32] = "3 m/s";         /* 风速：米/秒 */
static char g_windDir[32] = "Dir: NE";       /* 风向：N/NE/E/SE/S/SW/W/NW */
static char g_aqiStr[32] = "42";             /* 空气质量指数：0-500 */
static char g_aqiDesc[32] = "Good quality";  /* AQI 等级描述 */
static char g_pressureStr[32] = "1013";     /* 气压：百帕 (hPa) */
static char g_pressureDesc[32] = "Normal";   /* 气压描述：Low/Normal/High */
static char g_sunriseStr[32] = "06:30";      /* 日出时间 */
static char g_sunsetStr[32] = "18:30";       /* 日落时间 */
static char g_holidayName[64] = "Spring Festival";  /* 下一个节假日名称 */
static char g_holidayDays[32] = "15 days left";     /* 距离节假日的天数 */

/* 农历数据存储 */
static char g_lunarStr[32] = "1/23";         /* 农历日期：月/日 */
static char g_yearStr[32] = "Year of Horse"; /* 农历年份：生肖年 */

/**
 * @struct SolarTerm
 * @brief 二十四节气信息结构体
 *
 * 中国传统二十四节气，每个节气包含中文名、英文名、英文描述、日期
 * 注：日期为 2026 年的大约日期，实际节气时间每年略有不同
 */
typedef struct {
    const char *name;    /**< 中文名：如 "小寒" */
    const char *english; /**< 英文名：如 "Xiaohan" */
    const char *desc;    /**< 描述：如 "Minor Cold" */
    int month;           /**< 月份 (1-12) */
    int day;             /**< 日期 */
} SolarTerm;

/**
 * @brief 二十四节气数据表
 *
 * 每个节气大约相隔 15 天，分为 24 个等分点
 * 春季节气：立春、雨水、惊蛰、春分、清明、谷雨
 * 夏季节气：立夏、小满、芒种、夏至、小暑、大暑
 * 秋季节气：立秋、处暑、白露、秋分、寒露、霜降
 * 冬季节气：立冬、小雪、大雪、冬至、小寒、大寒
 */
static const SolarTerm solarTerms[] = {
    /* 春季节气 */
    {"小寒", "Xiaohan", "Minor Cold", 1, 5},
    {"大寒", "Dahan", "Major Cold", 1, 20},
    {"立春", "Lichun", "Spring begins", 2, 3},
    {"雨水", "Yushui", "Rain Water", 2, 18},
    {"惊蛰", "Jingzhe", "Awakening of Insects", 3, 5},
    {"春分", "Chunfen", "Spring Equinox", 3, 20},
    {"清明", "Qingming", "Pure Brightness", 4, 4},
    {"谷雨", "Guyu", "Grain Rain", 4, 19},
    /* 夏季节气 */
    {"立夏", "Lixia", "Summer begins", 5, 5},
    {"小满", "Xiaoman", "Grain Buds", 5, 20},
    {"芒种", "Mangzhong", "Grain in Ear", 6, 5},
    {"夏至", "Xiazhi", "Summer Solstice", 6, 21},
    {"小暑", "Xiaoshu", "Minor Heat", 7, 7},
    {"大暑", "Dashu", "Major Heat", 7, 22},
    /* 秋季节气 */
    {"立秋", "Liqiu", "Autumn begins", 8, 7},
    {"处暑", "Chushu", "Limit of Heat", 8, 22},
    {"白露", "Bailu", "White Dew", 9, 7},
    {"秋分", "Qiufen", "Autumn Equinox", 9, 22},
    {"寒露", "Hanlu", "Cold Dew", 10, 8},
    {"霜降", "Shuangjiang", "Frost Descent", 10, 23},
    /* 冬季节气 */
    {"立冬", "Lidong", "Winter begins", 11, 7},
    {"小雪", "Xiaoxue", "Minor Snow", 11, 22},
    {"大雪", "Daxue", "Major Snow", 12, 7},
    {"冬至", "Dongzhi", "Winter Solstice", 12, 21}
};

/* 当前/最近的节气 */
static char g_solarName[32] = "Jingzhe";      /* 节气英文名 */
static char g_solarDesc[32] = "Awakening of Insects";  /* 节气描述 */

/* ============================================================================
 * 节气计算函数
 * ============================================================================ */

/**
 * @brief 计算当前或下一个节气
 *
 * 根据当前月份和日期，查找最近的节气（当天或之后的第一个）
 * 使用一年中的第几天进行比较
 *
 * @param month 当前月份 (1-12)
 * @param day 当前日期 (1-31)
 */
static void GetSolarTerm(int month, int day)
{
    /* 各月份天数（非闰年） */
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    /* 计算今天是今年的第几天 */
    int currentDayOfYear = 0;
    for (int m = 1; m < month; m++) {
        currentDayOfYear += daysInMonth[m];
    }
    currentDayOfYear += day;

    /* 查找最近的节气 */
    int closestIdx = 0;
    int minDiff = 365;

    for (int i = 0; i < 24; i++) {
        /* 计算该节气是今年的第几天 */
        int termDay = 0;
        for (int m = 1; m < solarTerms[i].month; m++) {
            termDay += daysInMonth[m];
        }
        termDay += solarTerms[i].day;

        /* 计算与今天的差距（允许跨年） */
        int diff = termDay - currentDayOfYear;
        if (diff < 0) diff += 365;  /* 跨年处理 */

        if (diff < minDiff) {
            minDiff = diff;
            closestIdx = i;
        }
    }

    /* 保存最近的节气信息 */
    snprintf(g_solarName, sizeof(g_solarName), "%s", solarTerms[closestIdx].english);
    snprintf(g_solarDesc, sizeof(g_solarDesc), "%s", solarTerms[closestIdx].desc);
}

/* ============================================================================
 * HTTP 数据获取
 * ============================================================================ */

/**
 * @struct MemoryStruct
 * @brief libcurl 回调用的内存缓冲区
 *
 * 用于存储 HTTP 响应内容，动态分配内存
 */
struct MemoryStruct {
    char *memory;   /**< 数据缓冲区指针 */
    size_t size;    /**< 当前数据大小 */
};

/**
 * @brief libcurl 写回调函数
 *
 * 当收到 HTTP 响应数据时，libcurl 会调用此函数
 * 将数据追加到 MemoryStruct 结构体中
 *
 * @param contents 收到的数据指针
 * @param size 每个元素的大小
 * @param nmemb 元素数量
 * @param userp 用户数据指针（MemoryStruct）
 * @return 实际处理的字节数，失败返回 0
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    /* 扩展内存缓冲区 */
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;  /* 内存分配失败 */

    mem->memory = ptr;
    /* 追加新数据到缓冲区末尾 */
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;  /* 确保以 null 结尾 */

    return realsize;
}

/* ============================================================================
 * 天气数据获取
 * ============================================================================ */

/**
 * @brief 从 Open-Meteo API 获取天气数据
 *
 * 调用 Open-Meteo 免费 API 获取广州实时天气：
 * - 温度
 * - 湿度
 * - 风速和风向
 * - 气压
 * - 日出日落时间
 *
 * 广州坐标：23.1291°N, 113.2644°E
 *
 * @note API 官方文档：https://open-meteo.com/en/docs
 */
static void FetchWeatherData(void)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    /* 初始化内存缓冲区 */
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (!curl) return;

    /* Open-Meteo API URL（广州坐标） */
    const char *url = "https://api.open-meteo.com/v1/forecast?"
        "latitude=23.1291&longitude=113.2644"
        "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,pressure_msl"
        "&wind_speed_unit=ms"
        "&daily=sunrise,sunset"
        "&timezone=auto";

    /* 配置 curl 选项 */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);         /* 5秒超时 */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);   /* 跟随重定向 */

    res = curl_easy_perform(curl);

    if (res == CURLE_OK && chunk.memory) {
        /* JSON 解析 */
        struct json_object *parsed_json = json_tokener_parse(chunk.memory);
        if (parsed_json) {
            struct json_object *current, *temp, *humidity, *wind, *windDir;

            if (json_object_object_get_ex(parsed_json, "current", &current)) {
                /* 解析温度 */
                if (json_object_object_get_ex(current, "temperature_2m", &temp)) {
                    double t = json_object_get_double(temp);
                    snprintf(g_tempStr, sizeof(g_tempStr), "%.0f°C", t);
                }

                /* 解析湿度并生成描述 */
                if (json_object_object_get_ex(current, "relative_humidity_2m", &humidity)) {
                    int h = json_object_get_int(humidity);
                    snprintf(g_humidityStr, sizeof(g_humidityStr), "%d%%", h);
                    /* 根据湿度范围生成描述 */
                    if (h < 30) snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Dry");
                    else if (h < 50) snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Comfortable");
                    else if (h < 70) snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Humid");
                    else snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Very Humid");
                }

                /* 解析风速 */
                if (json_object_object_get_ex(current, "wind_speed_10m", &wind)) {
                    double w = json_object_get_double(wind);
                    snprintf(g_windStr, sizeof(g_windStr), "%.1f m/s", w);
                }

                /* 解析风向（角度转方位） */
                if (json_object_object_get_ex(current, "wind_direction_10m", &windDir)) {
                    int wd = json_object_get_int(windDir);
                    const char *dir = "N";
                    /* 角度转 8 个方位 */
                    if (wd >= 337 || wd < 22) dir = "N";
                    else if (wd < 67) dir = "NE";
                    else if (wd < 112) dir = "E";
                    else if (wd < 157) dir = "SE";
                    else if (wd < 202) dir = "S";
                    else if (wd < 247) dir = "SW";
                    else if (wd < 292) dir = "W";
                    else if (wd < 337) dir = "NW";
                    snprintf(g_windDir, sizeof(g_windDir), "Dir: %s", dir);
                }

                /* 解析气压并生成描述 */
                struct json_object *pressure;
                if (json_object_object_get_ex(current, "pressure_msl", &pressure)) {
                    double p = json_object_get_double(pressure);
                    snprintf(g_pressureStr, sizeof(g_pressureStr), "%.0f", p);
                    /* 气压范围描述 */
                    if (p < 1000) snprintf(g_pressureDesc, sizeof(g_pressureDesc), "Low");
                    else if (p < 1020) snprintf(g_pressureDesc, sizeof(g_pressureDesc), "Normal");
                    else snprintf(g_pressureDesc, sizeof(g_pressureDesc), "High");
                }
            }

            /* 解析日出日落时间 */
            struct json_object *daily, *sunrise, *sunset;
            if (json_object_object_get_ex(parsed_json, "daily", &daily)) {
                if (json_object_object_get_ex(daily, "sunrise", &sunrise)) {
                    struct json_object *sunriseArr = json_object_array_get_idx(sunrise, 0);
                    if (sunriseArr) {
                        const char *sr = json_object_get_string(sunriseArr);
                        if (sr && strlen(sr) >= 5) {
                            /* 从 ISO 8601 格式提取 HH:MM */
                            snprintf(g_sunriseStr, sizeof(g_sunriseStr), "%c%c:%c%c",
                                     sr[11], sr[12], sr[14], sr[15]);
                        }
                    }
                }
                if (json_object_object_get_ex(daily, "sunset", &sunset)) {
                    struct json_object *sunsetArr = json_object_array_get_idx(sunset, 0);
                    if (sunsetArr) {
                        const char *ss = json_object_get_string(sunsetArr);
                        if (ss && strlen(ss) >= 5) {
                            snprintf(g_sunsetStr, sizeof(g_sunsetStr), "%c%c:%c%c",
                                     ss[11], ss[12], ss[14], ss[15]);
                        }
                    }
                }
            }
            json_object_put(parsed_json);  /* 释放 JSON 对象 */
        }
        free(chunk.memory);
    }

    curl_easy_cleanup(curl);
}

/**
 * @brief 从 WAQI API 获取空气质量指数
 *
 * 调用 World Air Quality Index API 获取广州实时 AQI 值
 * AQI 范围：0-500，数值越低空气质量越好
 *
 * 分级：
 * - 0-50: Good (优)
 * - 51-100: Moderate (良)
 * - 101-150: Unhealthy for Sensitive Groups (轻度污染)
 * - 151+: Unhealthy (污染)
 *
 * @note 使用 demo token，功能受限但可正常使用
 */
static void FetchAQIData(void)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (!curl) return;

    /* WAQI API URL（广州） */
    const char *url = "https://api.waqi.info/feed/guangzhou/?token=demo";

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);

    if (res == CURLE_OK && chunk.memory) {
        struct json_object *parsed_json = json_tokener_parse(chunk.memory);
        if (parsed_json) {
            struct json_object *data, *aqi;

            if (json_object_object_get_ex(parsed_json, "data", &data)) {
                if (json_object_object_get_ex(data, "aqi", &aqi)) {
                    int aqiVal = json_object_get_int(aqi);
                    snprintf(g_aqiStr, sizeof(g_aqiStr), "%d", aqiVal);

                    /* AQI 等级判断 */
                    if (aqiVal <= 50) snprintf(g_aqiDesc, sizeof(g_aqiDesc), "Good");
                    else if (aqiVal <= 100) snprintf(g_aqiDesc, sizeof(g_aqiDesc), "Moderate");
                    else if (aqiVal <= 150) snprintf(g_aqiDesc, sizeof(g_aqiDesc), "Unhealthy SG");
                    else snprintf(g_aqiDesc, sizeof(g_aqiDesc), "Unhealthy");
                }
            }
            json_object_put(parsed_json);
        }
        free(chunk.memory);
    }

    curl_easy_cleanup(curl);
}

/**
 * @brief 计算下一个节假日
 *
 * 计算距离下一个中国法定节假日的天数
 * 包含：植树节、清明节、劳动节、端午节、中秋节、国庆节
 *
 * @note 节假日日期为近似值，实际放假安排可能不同
 */
static void FetchHolidays(void)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int currentDayOfYear = t->tm_yday;
    int currentYear = t->tm_year + 1900;

    /* 节假日结构体 */
    typedef struct {
        const char *name;
        int month;
        int day;
        int dayOfYear;
    } Holiday;

    /* 计算各节假日的年中日期 */
    struct tm hTm = {0};
    hTm.tm_year = currentYear - 1900;

    hTm.tm_mon = 2;  hTm.tm_mday = 12; mktime(&hTm);  /* 植树节 3月12日 */
    int arborDay = hTm.tm_yday;

    hTm.tm_mon = 3; hTm.tm_mday = 5; mktime(&hTm);  /* 清明节 4月5日 */
    int qingmingDay = hTm.tm_yday;

    hTm.tm_mon = 4; hTm.tm_mday = 1; mktime(&hTm);  /* 劳动节 5月1日 */
    int laborDay = hTm.tm_yday;

    hTm.tm_mon = 5; hTm.tm_mday = 19; mktime(&hTm);  /* 端午节 6月19日 */
    int dragonBoatDay = hTm.tm_yday;

    hTm.tm_mon = 8; hTm.tm_mday = 25; mktime(&hTm);  /* 中秋节 9月25日 */
    int midAutumnDay = hTm.tm_yday;

    hTm.tm_mon = 9; hTm.tm_mday = 1; mktime(&hTm);  /* 国庆节 10月1日 */
    int nationalDay = hTm.tm_yday;

    /* 节假日列表 */
    Holiday holidays[] = {
        {"Arbor Day", 2, 12, arborDay},
        {"Qingming", 3, 5, qingmingDay},
        {"Labor Day", 4, 1, laborDay},
        {"Dragon Boat", 5, 19, dragonBoatDay},
        {"Mid-Autumn", 8, 25, midAutumnDay},
        {"National Day", 9, 1, nationalDay}
    };

    /* 查找下一个最近的节假日 */
    int nextIdx = -1;
    int minDays = 365;

    for (int i = 0; i < 6; i++) {
        int daysLeft = holidays[i].dayOfYear - currentDayOfYear;
        if (daysLeft > 0 && daysLeft < minDays) {
            minDays = daysLeft;
            nextIdx = i;
        }
    }

    if (nextIdx >= 0) {
        snprintf(g_holidayName, sizeof(g_holidayName), "%s", holidays[nextIdx].name);
        snprintf(g_holidayDays, sizeof(g_holidayDays), "%d days left", minDays);
    } else {
        snprintf(g_holidayName, sizeof(g_holidayName), "New Year");
        snprintf(g_holidayDays, sizeof(g_holidayDays), "Next year");
    }
}

/**
 * @brief 获取农历日期（占位实现）
 *
 * @param year 公历年份
 * @param month 公历月份
 * @param day 公历日期
 *
 * @note 当前为简化实现，返回固定值
 *       完整实现需要农历计算库
 */
static void FetchLunarDate(int year, int month, int day)
{
    /* 简化实现：返回固定农历日期 */
    snprintf(g_lunarStr, sizeof(g_lunarStr), "1/23");
    snprintf(g_yearStr, sizeof(g_yearStr), "Year of Horse");
}

/* ============================================================================
 * 绘图工具函数
 * ============================================================================ */

/**
 * @brief 绘制圆角矩形
 *
 * 通过组合矩形和四个圆角来绘制圆角矩形
 * 用于绘制卡片和面板背景
 *
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param radius 圆角半径
 * @param color 填充颜色
 */
static void DrawRoundedRect(float x, float y, float w, float h, float radius, const SDL_Color *color)
{
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);

    /* 中心矩形区域 */
    SDL_FRect centerRect = {x + radius, y, w - 2*radius, h};
    SDL_RenderFillRect(renderer, &centerRect);

    /* 左侧矩形 */
    SDL_FRect leftRect = {x, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(renderer, &leftRect);

    /* 右侧矩形 */
    SDL_FRect rightRect = {x + w - radius, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(renderer, &rightRect);

    /* 绘制四个圆角（逐像素填充圆形） */
    int r = (int)radius;

    /* 左上角 */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + r - dx), (int)(y + r - dy));
            }
        }
    }
    /* 右上角 */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + w - r + dx), (int)(y + r - dy));
            }
        }
    }
    /* 左下角 */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + r - dx), (int)(y + h - r + dy));
            }
        }
    }
    /* 右下角 */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + w - r + dx), (int)(y + h - r + dy));
            }
        }
    }
}

/**
 * @brief 绘制卡片（圆角矩形便捷函数）
 *
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param w 宽度
 * @param h 高度
 * @param color 填充颜色
 */
static void DrawCard(float x, float y, float w, float h, const SDL_Color *color)
{
    DrawRoundedRect(x, y, w, h, 12, color);  /* 固定 12px 圆角 */
}

/**
 * @brief 渲染文字到屏幕
 *
 * 使用 SDL_ttf 将文字渲染为纹理并绘制
 *
 * @param text 要渲染的文字（UTF-8编码）
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param f 字体
 * @param color 文字颜色
 */
static void RenderText(const char *text, float x, float y, TTF_Font *f, const SDL_Color *color)
{
    if (!f || !text || !renderer) return;

    /* 渲染文字到 Surface */
    SDL_Surface *surface = TTF_RenderText_Blended(f, text, strlen(text), *color);
    if (!surface) return;

    /* 创建纹理 */
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return;
    }

    /* 绘制纹理到渲染器 */
    SDL_FRect dstRect = {x, y, (float)surface->w, (float)surface->h};
    SDL_RenderTexture(renderer, texture, NULL, &dstRect);

    /* 清理资源 */
    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

/**
 * @brief 绘制圆形边框
 *
 * 通过角度遍历绘制圆周上的点形成边框
 *
 * @param cx 圆心 X 坐标
 * @param cy 圆心 Y 坐标
 * @param radius 半径
 * @param color 边框颜色
 * @param thickness 边框粗细（像素）
 */
static void DrawCircleOutline(float cx, float cy, float radius, const SDL_Color *color, int thickness)
{
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);

    /* 绘制多层圆形成粗边框 */
    for (int t = 0; t < thickness; t++) {
        float r = radius - t;
        for (float angle = 0; angle < 360; angle += 0.5f) {
            float rad = angle * (float)M_PI / 180.0f;
            int px = (int)(cx + cosf(rad) * r);
            int py = (int)(cy + sinf(rad) * r);
            SDL_RenderPoint(renderer, px, py);
        }
    }
}

/**
 * @brief 绘制实心圆
 *
 * 遍历正方形区域，只绘制圆内的点
 *
 * @param cx 圆心 X 坐标
 * @param cy 圆心 Y 坐标
 * @param radius 半径
 * @param color 填充颜色
 */
static void FillCircle(float cx, float cy, float radius, const SDL_Color *color)
{
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);

    /* 逐像素填充：只绘制距离圆心小于半径的点 */
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderPoint(renderer, (int)(cx + x), (int)(cy + y));
            }
        }
    }
}

/* ============================================================================
 * UI 组件绘制
 * ============================================================================ */

/**
 * @brief 绘制模拟时钟
 *
 * 绘制带有时分秒针的圆形时钟，实时更新
 *
 * @param cx 圆心 X 坐标
 * @param cy 圆心 Y 坐标
 * @param radius 时钟半径
 */
static void DrawClock(float cx, float cy, float radius)
{
    /* 时钟背景 */
    SDL_Color faceColor = {20, 25, 50, 255};
    FillCircle(cx, cy, radius, &faceColor);

    /* 外圈边框 - 紫蓝色渐变效果 */
    SDL_Color borderOuter = {120, 100, 220, 255};
    DrawCircleOutline(cx, cy, radius, &borderOuter, 3);

    /* 内圈边框 */
    SDL_Color borderInner = {80, 60, 180, 200};
    DrawCircleOutline(cx, cy, radius - 5, &borderInner, 2);

    /* 12个小时刻度 */
    SDL_Color markColor = {220, 220, 220, 255};
    for (int i = 0; i < 12; i++) {
        float angle = (i * 30 - 90) * (float)M_PI / 180.0f;  /* 从12点钟方向开始 */
        /* 整点刻度更长 */
        float innerR = (i % 3 == 0) ? radius - 25 : radius - 15;
        float outerR = radius - 3;

        float x1 = cx + innerR * cosf(angle);
        float y1 = cy + innerR * sinf(angle);
        float x2 = cx + outerR * cosf(angle);
        float y2 = cy + outerR * sinf(angle);

        SDL_SetRenderDrawColor(renderer, markColor.r, markColor.g, markColor.b, markColor.a);
        int thickness = (i % 3 == 0) ? 3 : 2;
        for (int t = 0; t < thickness; t++) {
            SDL_RenderLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
        }
    }

    /* 获取当前时间 */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int hour = t->tm_hour % 12;
    int minute = t->tm_min;
    int second = t->tm_sec;

    /* 时针 - 粗紫色 */
    float hourAngle = (hour * 30 + minute * 0.5f - 90) * (float)M_PI / 180.0f;
    float hourLen = radius * 0.5f;
    float hx = cx + hourLen * cosf(hourAngle);
    float hy = cy + hourLen * sinf(hourAngle);
    SDL_Color hourColor = {200, 150, 255, 255};
    SDL_SetRenderDrawColor(renderer, hourColor.r, hourColor.g, hourColor.b, hourColor.a);
    for (int t = -2; t <= 2; t++) {
        SDL_RenderLine(renderer, (int)cx, (int)cy + t, (int)hx, (int)hy + t);
    }

    /* 分针 - 较长青色 */
    float minAngle = (minute * 6 - 90) * (float)M_PI / 180.0f;
    float minLen = radius * 0.75f;
    float mx = cx + minLen * cosf(minAngle);
    float my = cy + minLen * sinf(minAngle);
    SDL_Color minColor = {100, 200, 255, 255};
    SDL_SetRenderDrawColor(renderer, minColor.r, minColor.g, minColor.b, minColor.a);
    for (int t = -1; t <= 1; t++) {
        SDL_RenderLine(renderer, (int)cx, (int)cy + t, (int)mx, (int)my + t);
    }

    /* 秒针 - 细红色 */
    float secAngle = (second * 6 - 90) * (float)M_PI / 180.0f;
    float secLen = radius * 0.85f;
    float sx = cx + secLen * cosf(secAngle);
    float sy = cy + secLen * sinf(secAngle);
    SDL_Color secColor = {255, 80, 80, 255};
    SDL_SetRenderDrawColor(renderer, secColor.r, secColor.g, secColor.b, secColor.a);
    SDL_RenderLine(renderer, (int)cx, (int)cy, (int)sx, (int)sy);

    /* 中心圆点 - 白色带光晕 */
    SDL_Color centerColor = {255, 255, 255, 255};
    FillCircle(cx, cy, 6, &centerColor);
    SDL_Color centerGlow = {200, 200, 255, 150};
    DrawCircleOutline(cx, cy, 8, &centerGlow, 2);
}

/**
 * @brief 绘制天气图标（多云转雨）
 *
 * 绘制太阳和云朵组合的图标，带雨滴效果
 *
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param scale 缩放比例（1.0 = 原始大小）
 */
static void DrawWeatherIcon(float x, float y, float scale)
{
    /* 太阳 - 在云层后面 */
    SDL_Color sunColor = {255, 200, 50, 255};
    FillCircle(x + 40*scale, y + 30*scale, 22*scale, &sunColor);

    /* 太阳光芒（8条） */
    SDL_SetRenderDrawColor(renderer, sunColor.r, sunColor.g, sunColor.b, 180);
    for (int i = 0; i < 8; i++) {
        float angle = i * 45 * (float)M_PI / 180.0f;
        float r1 = 28*scale, r2 = 35*scale;
        float x1 = x + 40*scale + cosf(angle) * r1;
        float y1 = y + 30*scale + sinf(angle) * r1;
        float x2 = x + 40*scale + cosf(angle) * r2;
        float y2 = y + 30*scale + sinf(angle) * r2;
        SDL_RenderLine(renderer, (int)x1, (int)y1, (int)x2, (int)y2);
    }

    /* 云朵 - 多个重叠圆形 */
    SDL_Color cloudColor = {200, 200, 220, 255};
    FillCircle(x + 30*scale, y + 55*scale, 28*scale, &cloudColor);
    FillCircle(x + 65*scale, y + 48*scale, 35*scale, &cloudColor);
    FillCircle(x + 100*scale, y + 55*scale, 28*scale, &cloudColor);
    FillCircle(x + 50*scale, y + 35*scale, 25*scale, &cloudColor);
    FillCircle(x + 85*scale, y + 38*scale, 22*scale, &cloudColor);

    /* 雨滴 */
    SDL_Color rainColor = {100, 150, 255, 255};
    FillCircle(x + 45*scale, y + 95*scale, 5*scale, &rainColor);
    FillCircle(x + 70*scale, y + 95*scale, 5*scale, &rainColor);
    FillCircle(x + 95*scale, y + 95*scale, 5*scale, &rainColor);
}

/**
 * @brief 绘制信息卡片
 *
 * 显示标题、数值和副标题的卡片式组件
 *
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 * @param title 标题（小字灰色）
 * @param value 数值（中字白色）
 * @param subtitle 副标题（小字青色）
 * @param cardColor 卡片背景颜色
 */
static void DrawInfoCard(float x, float y, const char *title, const char *value, const char *subtitle, const SDL_Color *cardColor)
{
    DrawCard(x, y, 165, 145, cardColor);
    RenderText(title, x + 12, y + 12, font_small, &COLOR_GRAY);
    RenderText(value, x + 12, y + 55, font_medium, &COLOR_WHITE);
    RenderText(subtitle, x + 12, y + 110, font_small, &COLOR_CYAN);
}

/**
 * @brief 绘制日历面板
 *
 * 显示月份、大号日期、星期、完整月历网格和节假日信息
 *
 * @param x 左上角 X 坐标
 * @param y 左上角 Y 坐标
 */
static void DrawCalendar(float x, float y)
{
    DrawCard(x, y, 280, 480, &COLOR_PANEL_BG);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    /* 月份和年份标题 */
    char monthStr[32];
    strftime(monthStr, sizeof(monthStr), "%B %Y", t);
    RenderText(monthStr, x + 20, y + 25, font_medium, &COLOR_WHITE);

    /* 大号日期数字 */
    char dayStr[8];
    snprintf(dayStr, sizeof(dayStr), "%d", t->tm_mday);
    RenderText(dayStr, x + 20, y + 75, font_large, &COLOR_ORANGE);

    /* 星期缩写 */
    char weekdayStr[16];
    strftime(weekdayStr, sizeof(weekdayStr), "%a", t);
    RenderText(weekdayStr, x + 120, y + 95, font_medium, &COLOR_GRAY);

    /* 星期表头 */
    const char *weekdays[] = {"S", "M", "T", "W", "T", "F", "S"};
    int colWidth = 36;
    int startX = 15;
    for (int i = 0; i < 7; i++) {
        SDL_Color c = (i == 0 || i == 6) ? COLOR_PURPLE : COLOR_WHITE;
        RenderText(weekdays[i], x + startX + i * colWidth, y + 145, font_small, &c);
    }

    /* 计算当月第一天和总天数 */
    struct tm monthStart = *t;
    monthStart.tm_mday = 1;
    mktime(&monthStart);

    struct tm nextMonth = monthStart;
    nextMonth.tm_mon++;
    mktime(&nextMonth);
    nextMonth.tm_mday = 0;
    mktime(&nextMonth);
    int daysInMonth = nextMonth.tm_mday;

    /* 绘制月历网格 */
    int row = 0;
    for (int day = 1; day <= daysInMonth; day++) {
        struct tm dayTm = *t;
        dayTm.tm_mday = day;
        mktime(&dayTm);
        int dow = dayTm.tm_wday;

        if (dow == 0 && day > 1) row++;

        char numStr[4];
        snprintf(numStr, sizeof(numStr), "%d", day);

        /* 日期颜色：当天=橙色，周末=紫色，工作日=白色 */
        SDL_Color c;
        if (day == t->tm_mday) {
            c = COLOR_ORANGE;
        } else if (dow == 0 || dow == 6) {
            c = COLOR_PURPLE;
        } else {
            c = COLOR_WHITE;
        }

        float numX = x + startX + dow * colWidth;
        float numY = y + 180 + row * 38;
        RenderText(numStr, numX, numY, font_small, &c);
    }

    /* 节假日信息 */
    float holidayY = y + 180 + (row + 1) * 38 + 10;
    RenderText("Next Holiday:", x + 20, holidayY, font_small, &COLOR_CYAN);
    RenderText(g_holidayName, x + 20, holidayY + 22, font_small, &COLOR_WHITE);
    RenderText(g_holidayDays, x + 20, holidayY + 42, font_small, &COLOR_ORANGE);
}

/* ============================================================================
 * SDL 回调函数
 * ============================================================================ */

/**
 * @brief SDL 应用初始化回调
 *
 * 在应用启动时调用一次，负责：
 * - 初始化 SDL 视频、字体子系统
 * - 创建窗口和渲染器
 * - 加载字体
 * - 初始化 curl
 * - 获取初始数据
 *
 * @param appstate 应用状态指针（未使用）
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return SDL_APP_CONTINUE 继续运行，SDL_APP_FAILURE 初始化失败
 */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Dashboard Clock", "1.0", "com.example.dashboard");

    /* 初始化 SDL 视频子系统 */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* 创建窗口和渲染器 */
    if (!SDL_CreateWindowAndRenderer("Dashboard", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* 初始化 SDL_ttf */
    if (!TTF_Init()) {
        SDL_Log("TTF init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* 尝试加载多种字体（兼容不同发行版） */
    const char *fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",  /* Debian/Ubuntu */
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",  /* Fedora/RHEL */
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",  /* Arch Linux */
        NULL
    };

    for (int i = 0; fontPaths[i]; i++) {
        if (!font_large) font_large = TTF_OpenFont(fontPaths[i], 72);
        if (!font_medium) font_medium = TTF_OpenFont(fontPaths[i], 32);
        if (!font_small) font_small = TTF_OpenFont(fontPaths[i], 18);
    }

    if (!font_large || !font_medium || !font_small) {
        SDL_Log("Warning: Some fonts failed to load");
    }

    /* 初始化 libcurl */
    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* 获取初始数据 */
    FetchWeatherData();    /* 天气 */
    FetchAQIData();        /* 空气质量 */
    FetchHolidays();       /* 节假日 */

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    FetchLunarDate(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    GetSolarTerm(t->tm_mon + 1, t->tm_mday);

    SDL_HideCursor();  /* 隐藏鼠标光标（全屏仪表板模式） */
    return SDL_APP_CONTINUE;
}

/**
 * @brief SDL 事件处理回调
 *
 * 处理用户输入事件
 *
 * @param appstate 应用状态指针（未使用）
 * @param event SDL 事件结构体
 * @return SDL_APP_SUCCESS 请求退出，SDL_APP_CONTINUE 继续运行
 */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    /* 窗口关闭事件 */
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    /* ESC 键退出 */
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

/**
 * @brief SDL 主循环回调
 *
 * 每帧调用一次，负责绘制整个界面
 *
 * @param appstate 应用状态指针（未使用）
 * @return SDL_APP_CONTINUE 继续运行
 */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    /* 绘制背景渐变 */
    SDL_SetRenderDrawColor(renderer, 20, 30, 60, 255);
    SDL_RenderClear(renderer);

    /* 从上到下的渐变条带 */
    for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
        float ratio = (float)y / SCREEN_HEIGHT;
        SDL_Color c = {
            .r = (int)(40 * (1-ratio) + 15 * ratio),  /* 上浅下深 */
            .g = (int)(50 * (1-ratio) + 25 * ratio),
            .b = (int)(90 * (1-ratio) + 50 * ratio),
            .a = 255
        };
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_FRect rect = {0, y, SCREEN_WIDTH, 4};
        SDL_RenderFillRect(renderer, &rect);
    }

    /* 主面板（左侧） */
    DrawCard(20, 80, 940, 600, &COLOR_PANEL_BG);

    /* 获取当前时间 */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    /* 温度显示 */
    RenderText(g_tempStr, 50, 150, font_large, &COLOR_WHITE);

    /* 位置 */
    RenderText("Guangzhou/CN", 50, 250, font_medium, &COLOR_GRAY);

    /* 天气图标 */
    DrawWeatherIcon(340, 170, 1.2f);

    /* 模拟时钟 */
    DrawClock(670, 310, 140);

    /* 信息卡片行 */
    DrawInfoCard(70, 530, "Humidity", g_humidityStr, g_humidityDesc, &COLOR_CARD_1);
    DrawInfoCard(240, 530, "Wind", g_windStr, g_windDir, &COLOR_CARD_2);
    DrawInfoCard(410, 530, "AQI", g_aqiStr, g_aqiDesc, &COLOR_CARD_3);
    DrawInfoCard(580, 530, "Pressure", g_pressureStr, g_pressureDesc, &COLOR_CARD_4);
    DrawInfoCard(750, 530, "Sunrise", g_sunriseStr, g_sunsetStr, &COLOR_CARD_5);

    /* 日历面板（右侧） */
    DrawCalendar(970, 110);

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

/**
 * @brief SDL 应用退出回调
 *
 * 在应用退出时调用，负责清理所有资源
 *
 * @param appstate 应用状态指针（未使用）
 * @param result 退出原因
 */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* 清理字体 */
    if (font_large) TTF_CloseFont(font_large);
    if (font_medium) TTF_CloseFont(font_medium);
    if (font_small) TTF_CloseFont(font_small);

    /* 清理 SDL 资源 */
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);

    /* 退出 SDL 子系统 */
    TTF_Quit();
    SDL_Quit();

    /* 清理 curl */
    curl_global_cleanup();
}
