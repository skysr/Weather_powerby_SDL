#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3/SDL_main.h>

/* SDL_image function declaration */
extern SDL_DECLSPEC SDL_Texture * SDLCALL IMG_LoadTexture(SDL_Renderer *renderer, const char *file);

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <json-c/json.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 800

/* Colors */
static const SDL_Color COLOR_WHITE = {255, 255, 255, 255};
static const SDL_Color COLOR_GRAY = {180, 180, 180, 255};
static const SDL_Color COLOR_CYAN = {100, 200, 255, 255};
static const SDL_Color COLOR_PURPLE = {180, 100, 255, 255};
static const SDL_Color COLOR_ORANGE = {255, 150, 50, 255};
static const SDL_Color COLOR_BLUE = {100, 150, 255, 255};

/* Panel colors */
static const SDL_Color COLOR_PANEL_BG = {30, 35, 60, 230};
static const SDL_Color COLOR_CARD_1 = {30, 50, 90, 200};
static const SDL_Color COLOR_CARD_2 = {30, 60, 100, 200};
static const SDL_Color COLOR_CARD_3 = {40, 70, 110, 200};
static const SDL_Color COLOR_CARD_4 = {60, 50, 120, 200};
static const SDL_Color COLOR_CARD_5 = {100, 50, 80, 200};

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static TTF_Font *font_large = NULL;
static TTF_Font *font_medium = NULL;
static TTF_Font *font_small = NULL;
static SDL_Texture *bgTexture = NULL;
static time_t lastWallpaperFetch = 0;
static const int WALLPAPER_UPDATE_INTERVAL = 300;  /* 5 minutes */

/* Weather data storage */
static char g_tempStr[32] = "24°C";
static char g_humidityStr[32] = "65%";
static char g_humidityDesc[32] = "Comfortable";
static char g_windStr[32] = "3 m/s";
static char g_windDir[32] = "Dir: NE";
static char g_aqiStr[32] = "42";
static char g_aqiDesc[32] = "Good quality";
static char g_pressureStr[32] = "1013";
static char g_pressureDesc[32] = "Normal";
static char g_sunriseStr[32] = "06:30";
static char g_sunsetStr[32] = "18:30";
static char g_holidayName[64] = "Spring Festival";
static char g_holidayDays[32] = "15 days left";

/* Lunar data storage */
static char g_lunarStr[32] = "1/23";
static char g_yearStr[32] = "Year of Horse";

/* Solar Terms (24节气) - approximate dates for 2026 */
typedef struct {
    const char *name;
    const char *english;
    const char *desc;
    int month;
    int day;
} SolarTerm;

static const SolarTerm solarTerms[] = {
    {"小寒", "Xiaohan", "Minor Cold", 1, 5},
    {"大寒", "Dahan", "Major Cold", 1, 20},
    {"立春", "Lichun", "Spring begins", 2, 3},
    {"雨水", "Yushui", "Rain Water", 2, 18},
    {"惊蛰", "Jingzhe", "Awakening of Insects", 3, 5},
    {"春分", "Chunfen", "Spring Equinox", 3, 20},
    {"清明", "Qingming", "Pure Brightness", 4, 4},
    {"谷雨", "Guyu", "Grain Rain", 4, 19},
    {"立夏", "Lixia", "Summer begins", 5, 5},
    {"小满", "Xiaoman", "Grain Buds", 5, 20},
    {"芒种", "Mangzhong", "Grain in Ear", 6, 5},
    {"夏至", "Xiazhi", "Summer Solstice", 6, 21},
    {"小暑", "Xiaoshu", "Minor Heat", 7, 7},
    {"大暑", "Dashu", "Major Heat", 7, 22},
    {"立秋", "Liqiu", "Autumn begins", 8, 7},
    {"处暑", "Chushu", "Limit of Heat", 8, 22},
    {"白露", "Bailu", "White Dew", 9, 7},
    {"秋分", "Qiufen", "Autumn Equinox", 9, 22},
    {"寒露", "Hanlu", "Cold Dew", 10, 8},
    {"霜降", "Shuangjiang", "Frost Descent", 10, 23},
    {"立冬", "Lidong", "Winter begins", 11, 7},
    {"小雪", "Xiaoxue", "Minor Snow", 11, 22},
    {"大雪", "Daxue", "Major Snow", 12, 7},
    {"冬至", "Dongzhi", "Winter Solstice", 12, 21}
};

static char g_solarName[32] = "Jingzhe";
static char g_solarDesc[32] = "Awakening of Insects";

/* Get current/next solar term */
static void GetSolarTerm(int month, int day)
{
    int currentDayOfYear = 0;
    int daysInMonth[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    for (int m = 1; m < month; m++) {
        currentDayOfYear += daysInMonth[m];
    }
    currentDayOfYear += day;
    
    /* Find the closest solar term */
    int closestIdx = 0;
    int minDiff = 365;
    
    for (int i = 0; i < 24; i++) {
        int termDay = 0;
        for (int m = 1; m < solarTerms[i].month; m++) {
            termDay += daysInMonth[m];
        }
        termDay += solarTerms[i].day;
        
        int diff = termDay - currentDayOfYear;
        if (diff < 0) diff += 365;  /* Wrap around year */
        
        if (diff < minDiff) {
            minDiff = diff;
            closestIdx = i;
        }
    }
    
    snprintf(g_solarName, sizeof(g_solarName), "%s", solarTerms[closestIdx].english);
    snprintf(g_solarDesc, sizeof(g_solarDesc), "%s", solarTerms[closestIdx].desc);
}

/* HTTP response buffer for curl */
struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) return 0;
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

/* Fetch weather data from Open-Meteo API (Guangzhou: 23.1291°N, 113.2644°E) */
static void FetchWeatherData(void)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl = curl_easy_init();
    if (!curl) return;
    
    /* Open-Meteo API for Guangzhou - with daily sunrise/sunset */
    const char *url = "https://api.open-meteo.com/v1/forecast?latitude=23.1291&longitude=113.2644&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m,wind_direction_10m,pressure_msl&wind_speed_unit=ms&daily=sunrise,sunset&timezone=auto";
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    res = curl_easy_perform(curl);
    
    if (res == CURLE_OK && chunk.memory) {
        struct json_object *parsed_json = json_tokener_parse(chunk.memory);
        if (parsed_json) {
            struct json_object *current, *temp, *humidity, *wind, *windDir;
            
            if (json_object_object_get_ex(parsed_json, "current", &current)) {
                /* Temperature */
                if (json_object_object_get_ex(current, "temperature_2m", &temp)) {
                    double t = json_object_get_double(temp);
                    snprintf(g_tempStr, sizeof(g_tempStr), "%.0f°C", t);
                }
                
                /* Humidity */
                if (json_object_object_get_ex(current, "relative_humidity_2m", &humidity)) {
                    int h = json_object_get_int(humidity);
                    snprintf(g_humidityStr, sizeof(g_humidityStr), "%d%%", h);
                    if (h < 30) snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Dry");
                    else if (h < 50) snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Comfortable");
                    else if (h < 70) snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Humid");
                    else snprintf(g_humidityDesc, sizeof(g_humidityDesc), "Very Humid");
                }
                
                /* Wind speed */
                if (json_object_object_get_ex(current, "wind_speed_10m", &wind)) {
                    double w = json_object_get_double(wind);
                    snprintf(g_windStr, sizeof(g_windStr), "%.1f m/s", w);
                }
                
                /* Wind direction */
                if (json_object_object_get_ex(current, "wind_direction_10m", &windDir)) {
                    int wd = json_object_get_int(windDir);
                    const char *dir = "N";
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
                
                /* Pressure */
                struct json_object *pressure;
                if (json_object_object_get_ex(current, "pressure_msl", &pressure)) {
                    double p = json_object_get_double(pressure);
                    snprintf(g_pressureStr, sizeof(g_pressureStr), "%.0f", p);
                    if (p < 1000) snprintf(g_pressureDesc, sizeof(g_pressureDesc), "Low");
                    else if (p < 1020) snprintf(g_pressureDesc, sizeof(g_pressureDesc), "Normal");
                    else snprintf(g_pressureDesc, sizeof(g_pressureDesc), "High");
                }
            }
            
            /* Daily data - sunrise/sunset */
            struct json_object *daily, *sunrise, *sunset;
            if (json_object_object_get_ex(parsed_json, "daily", &daily)) {
                if (json_object_object_get_ex(daily, "sunrise", &sunrise)) {
                    struct json_object *sunriseArr = json_object_array_get_idx(sunrise, 0);
                    if (sunriseArr) {
                        const char *sr = json_object_get_string(sunriseArr);
                        if (sr && strlen(sr) >= 5) {
                            /* Extract HH:MM from ISO format */
                            snprintf(g_sunriseStr, sizeof(g_sunriseStr), "%c%c:%c%c", sr[11], sr[12], sr[14], sr[15]);
                        }
                    }
                }
                if (json_object_object_get_ex(daily, "sunset", &sunset)) {
                    struct json_object *sunsetArr = json_object_array_get_idx(sunset, 0);
                    if (sunsetArr) {
                        const char *ss = json_object_get_string(sunsetArr);
                        if (ss && strlen(ss) >= 5) {
                            snprintf(g_sunsetStr, sizeof(g_sunsetStr), "%c%c:%c%c", ss[11], ss[12], ss[14], ss[15]);
                        }
                    }
                }
            }
            json_object_put(parsed_json);
        }
        free(chunk.memory);
    }
    
    curl_easy_cleanup(curl);
}

/* Fetch AQI data from WAQI API (Guangzhou) */
static void FetchAQIData(void)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl = curl_easy_init();
    if (!curl) return;
    
    /* WAQI API for Guangzhou */
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

/* Fetch Bing daily wallpaper and load as background texture */
static void FetchBingWallpaper(void)
{
    CURL *curl;
    CURLcode res;
    struct MemoryStruct chunk;
    
    SDL_Log("Fetching Bing wallpaper...");
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl = curl_easy_init();
    if (!curl) {
        SDL_Log("Failed to init curl");
        return;
    }
    
    /* Step 1: Get Bing image info JSON */
    const char *jsonUrl = "https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1&mkt=en-US";
    
    curl_easy_setopt(curl, CURLOPT_URL, jsonUrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        SDL_Log("Bing JSON fetch failed: %s", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        free(chunk.memory);
        return;
    }
    
    SDL_Log("Bing JSON response: %zu bytes", chunk.size);
    
    if (chunk.memory && chunk.size > 0) {
        struct json_object *parsed_json = json_tokener_parse(chunk.memory);
        if (parsed_json) {
            struct json_object *images, *image, *urlObj;
            
            if (json_object_object_get_ex(parsed_json, "images", &images)) {
                image = json_object_array_get_idx(images, 0);
                if (image && json_object_object_get_ex(image, "url", &urlObj)) {
                    const char *imageUrl = json_object_get_string(urlObj);
                    SDL_Log("Bing image URL: %s", imageUrl);
                    
                    /* Build full URL */
                    char fullUrl[512];
                    snprintf(fullUrl, sizeof(fullUrl), "https://www.bing.com%s", imageUrl);
                    
                    /* Step 2: Download the image */
                    free(chunk.memory);
                    chunk.memory = malloc(1);
                    chunk.size = 0;
                    
                    curl_easy_setopt(curl, CURLOPT_URL, fullUrl);
                    res = curl_easy_perform(curl);
                    
                    if (res == CURLE_OK && chunk.size > 0) {
                        SDL_Log("Image downloaded: %zu bytes", chunk.size);
                        
                        /* Save to temp file */
                        FILE *fp = fopen("/tmp/bing_bg.jpg", "wb");
                        if (fp) {
                            fwrite(chunk.memory, 1, chunk.size, fp);
                            fclose(fp);
                            SDL_Log("Saved to /tmp/bing_bg.jpg");
                            
                            /* Destroy old texture */
                            if (bgTexture) SDL_DestroyTexture(bgTexture);
                            
                            /* Load image as texture */
                            bgTexture = IMG_LoadTexture(renderer, "/tmp/bing_bg.jpg");
                            if (bgTexture) {
                                SDL_Log("Wallpaper loaded successfully!");
                                
                                /* Get texture size for debugging */
                                float texW, texH;
                                SDL_GetTextureSize(bgTexture, &texW, &texH);
                                SDL_Log("Texture size: %.0fx%.0f", texW, texH);
                            } else {
                                SDL_Log("Failed to load texture: %s", SDL_GetError());
                                
                                /* Try to verify file exists */
                                FILE *test = fopen("/tmp/bing_bg.jpg", "rb");
                                if (test) {
                                    fclose(test);
                                    SDL_Log("File /tmp/bing_bg.jpg exists, IMG_LoadTexture failed");
                                } else {
                                    SDL_Log("File /tmp/bing_bg.jpg does not exist");
                                }
                            }
                        } else {
                            SDL_Log("Failed to save file");
                        }
                    } else {
                        SDL_Log("Image download failed: %s", curl_easy_strerror(res));
                    }
                }
            }
            json_object_put(parsed_json);
        }
        free(chunk.memory);
    }
    
    curl_easy_cleanup(curl);
}

/* Fetch Chinese holidays from API */
static void FetchHolidays(void)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int currentDayOfYear = t->tm_yday;
    int currentYear = t->tm_year + 1900;
    
    /* Define holidays for 2026 with day of year */
    typedef struct {
        const char *name;
        int month;  /* 0-11 */
        int day;
        int dayOfYear;
    } Holiday;
    
    /* Calculate day of year for each holiday */
    struct tm hTm = {0};
    hTm.tm_year = currentYear - 1900;
    hTm.tm_mon = 2;  hTm.tm_mday = 12; mktime(&hTm);  /* Arbor Day Mar 12 */
    int arborDay = hTm.tm_yday;
    
    hTm.tm_mon = 3; hTm.tm_mday = 5; mktime(&hTm);  /* Qingming Apr 5 */
    int qingmingDay = hTm.tm_yday;
    
    hTm.tm_mon = 4; hTm.tm_mday = 1; mktime(&hTm);  /* Labor Day May 1 */
    int laborDay = hTm.tm_yday;
    
    hTm.tm_mon = 5; hTm.tm_mday = 19; mktime(&hTm);  /* Dragon Boat Jun 19 */
    int dragonBoatDay = hTm.tm_yday;
    
    hTm.tm_mon = 8; hTm.tm_mday = 25; mktime(&hTm);  /* Mid-Autumn Sep 25 */
    int midAutumnDay = hTm.tm_yday;
    
    hTm.tm_mon = 9; hTm.tm_mday = 1; mktime(&hTm);  /* National Day Oct 1 */
    int nationalDay = hTm.tm_yday;
    
    /* Find next holiday */
    Holiday holidays[] = {
        {"Arbor Day", 2, 12, arborDay},
        {"Qingming", 3, 5, qingmingDay},
        {"Labor Day", 4, 1, laborDay},
        {"Dragon Boat", 5, 19, dragonBoatDay},
        {"Mid-Autumn", 8, 25, midAutumnDay},
        {"National Day", 9, 1, nationalDay}
    };
    
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

/* Fetch lunar date from web API */
static void FetchLunarDate(int year, int month, int day)
{
    /* Simplified: use static lunar date for now */
    /* 2026-03-11 ≈ Lunar 1/23 (正月二十三) */
    snprintf(g_lunarStr, sizeof(g_lunarStr), "1/23");
    snprintf(g_yearStr, sizeof(g_yearStr), "Year of Horse");
}

/* Draw a filled rounded rectangle with proper corners */
static void DrawRoundedRect(float x, float y, float w, float h, float radius, const SDL_Color *color)
{
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    
    /* Center rectangle */
    SDL_FRect centerRect = {x + radius, y, w - 2*radius, h};
    SDL_RenderFillRect(renderer, &centerRect);
    
    /* Left rectangle */
    SDL_FRect leftRect = {x, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(renderer, &leftRect);
    
    /* Right rectangle */
    SDL_FRect rightRect = {x + w - radius, y + radius, radius, h - 2*radius};
    SDL_RenderFillRect(renderer, &rightRect);
    
    /* Draw 4 corner circles (filled) */
    int r = (int)radius;
    /* Top-left corner */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + r - dx), (int)(y + r - dy));
            }
        }
    }
    /* Top-right corner */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + w - r + dx), (int)(y + r - dy));
            }
        }
    }
    /* Bottom-left corner */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + r - dx), (int)(y + h - r + dy));
            }
        }
    }
    /* Bottom-right corner */
    for (int dy = 0; dy <= r; dy++) {
        for (int dx = 0; dx <= r; dx++) {
            if (dx*dx + dy*dy <= r*r) {
                SDL_RenderPoint(renderer, (int)(x + w - r + dx), (int)(y + h - r + dy));
            }
        }
    }
}

/* Draw a card/panel with rounded corners */
static void DrawCard(float x, float y, float w, float h, const SDL_Color *color)
{
    DrawRoundedRect(x, y, w, h, 12, color);
}

/* Render text */
static void RenderText(const char *text, float x, float y, TTF_Font *f, const SDL_Color *color)
{
    if (!f || !text || !renderer) return;
    
    SDL_Surface *surface = TTF_RenderText_Blended(f, text, strlen(text), *color);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_DestroySurface(surface);
        return;
    }

    SDL_FRect dstRect = {x, y, (float)surface->w, (float)surface->h};
    SDL_RenderTexture(renderer, texture, NULL, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_DestroySurface(surface);
}

/* Draw circle outline */
static void DrawCircleOutline(float cx, float cy, float radius, const SDL_Color *color, int thickness)
{
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    
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

/* Fill circle */
static void FillCircle(float cx, float cy, float radius, const SDL_Color *color)
{
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x*x + y*y <= radius*radius) {
                SDL_RenderPoint(renderer, (int)(cx + x), (int)(cy + y));
            }
        }
    }
}

/* Draw analog clock */
static void DrawClock(float cx, float cy, float radius)
{
    /* Clock face background */
    SDL_Color faceColor = {20, 25, 50, 255};
    FillCircle(cx, cy, radius, &faceColor);
    
    /* Clock outer border - bright purple/blue gradient effect */
    SDL_Color borderOuter = {120, 100, 220, 255};
    DrawCircleOutline(cx, cy, radius, &borderOuter, 3);
    
    /* Inner border */
    SDL_Color borderInner = {80, 60, 180, 200};
    DrawCircleOutline(cx, cy, radius - 5, &borderInner, 2);
    
    /* Hour marks - longer and more visible */
    SDL_Color markColor = {220, 220, 220, 255};
    for (int i = 0; i < 12; i++) {
        float angle = (i * 30 - 90) * (float)M_PI / 180.0f;
        float innerR = (i % 3 == 0) ? radius - 25 : radius - 15;  /* Quarter hours longer */
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
    
    /* Get current time */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int hour = t->tm_hour % 12;
    int minute = t->tm_min;
    int second = t->tm_sec;
    
    /* Hour hand - thick purple */
    float hourAngle = (hour * 30 + minute * 0.5f - 90) * (float)M_PI / 180.0f;
    float hourLen = radius * 0.5f;
    float hx = cx + hourLen * cosf(hourAngle);
    float hy = cy + hourLen * sinf(hourAngle);
    SDL_Color hourColor = {200, 150, 255, 255};
    SDL_SetRenderDrawColor(renderer, hourColor.r, hourColor.g, hourColor.b, hourColor.a);
    for (int t = -2; t <= 2; t++) {
        SDL_RenderLine(renderer, (int)cx, (int)cy + t, (int)hx, (int)hy + t);
    }
    
    /* Minute hand - longer cyan */
    float minAngle = (minute * 6 - 90) * (float)M_PI / 180.0f;
    float minLen = radius * 0.75f;
    float mx = cx + minLen * cosf(minAngle);
    float my = cy + minLen * sinf(minAngle);
    SDL_Color minColor = {100, 200, 255, 255};
    SDL_SetRenderDrawColor(renderer, minColor.r, minColor.g, minColor.b, minColor.a);
    for (int t = -1; t <= 1; t++) {
        SDL_RenderLine(renderer, (int)cx, (int)cy + t, (int)mx, (int)my + t);
    }
    
    /* Second hand - thin red */
    float secAngle = (second * 6 - 90) * (float)M_PI / 180.0f;
    float secLen = radius * 0.85f;
    float sx = cx + secLen * cosf(secAngle);
    float sy = cy + secLen * sinf(secAngle);
    SDL_Color secColor = {255, 80, 80, 255};
    SDL_SetRenderDrawColor(renderer, secColor.r, secColor.g, secColor.b, secColor.a);
    SDL_RenderLine(renderer, (int)cx, (int)cy, (int)sx, (int)sy);
    
    /* Center dot - white with glow effect */
    SDL_Color centerColor = {255, 255, 255, 255};
    FillCircle(cx, cy, 6, &centerColor);
    SDL_Color centerGlow = {200, 200, 255, 150};
    DrawCircleOutline(cx, cy, 8, &centerGlow, 2);
}

/* Draw weather icon (cloud with sun) */
static void DrawWeatherIcon(float x, float y, float scale)
{
    /* Sun - behind cloud */
    SDL_Color sunColor = {255, 200, 50, 255};
    FillCircle(x + 40*scale, y + 30*scale, 22*scale, &sunColor);
    
    /* Sun rays */
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
    
    /* Cloud - multiple overlapping circles */
    SDL_Color cloudColor = {200, 200, 220, 255};
    FillCircle(x + 30*scale, y + 55*scale, 28*scale, &cloudColor);
    FillCircle(x + 65*scale, y + 48*scale, 35*scale, &cloudColor);
    FillCircle(x + 100*scale, y + 55*scale, 28*scale, &cloudColor);
    FillCircle(x + 50*scale, y + 35*scale, 25*scale, &cloudColor);
    FillCircle(x + 85*scale, y + 38*scale, 22*scale, &cloudColor);
    
    /* Rain drops */
    SDL_Color rainColor = {100, 150, 255, 255};
    FillCircle(x + 45*scale, y + 95*scale, 5*scale, &rainColor);
    FillCircle(x + 70*scale, y + 95*scale, 5*scale, &rainColor);
    FillCircle(x + 95*scale, y + 95*scale, 5*scale, &rainColor);
}

/* Draw info card */
static void DrawInfoCard(float x, float y, const char *title, const char *value, const char *subtitle, const SDL_Color *cardColor)
{
    DrawCard(x, y, 165, 145, cardColor);
    RenderText(title, x + 12, y + 12, font_small, &COLOR_GRAY);
    RenderText(value, x + 12, y + 55, font_medium, &COLOR_WHITE);
    RenderText(subtitle, x + 12, y + 110, font_small, &COLOR_CYAN);
}

/* Draw calendar */
static void DrawCalendar(float x, float y)
{
    DrawCard(x, y, 280, 480, &COLOR_PANEL_BG);
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    /* Month and year header */
    char monthStr[32];
    strftime(monthStr, sizeof(monthStr), "%B %Y", t);
    RenderText(monthStr, x + 20, y + 25, font_medium, &COLOR_WHITE);
    
    /* Large day number - LARGER font */
    char dayStr[8];
    snprintf(dayStr, sizeof(dayStr), "%d", t->tm_mday);
    RenderText(dayStr, x + 20, y + 75, font_large, &COLOR_ORANGE);
    
    /* Weekday - abbreviated format, moved UP */
    char weekdayStr[16];
    strftime(weekdayStr, sizeof(weekdayStr), "%a", t);
    RenderText(weekdayStr, x + 120, y + 95, font_medium, &COLOR_GRAY);
    
    /* Week day headers - moved down */
    const char *weekdays[] = {"S", "M", "T", "W", "T", "F", "S"};
    int colWidth = 36;
    int startX = 15;
    for (int i = 0; i < 7; i++) {
        SDL_Color c = (i == 0 || i == 6) ? COLOR_PURPLE : COLOR_WHITE;
        RenderText(weekdays[i], x + startX + i * colWidth, y + 145, font_small, &c);
    }
    
    /* Calendar grid - moved down */
    struct tm monthStart = *t;
    monthStart.tm_mday = 1;
    mktime(&monthStart);
    
    struct tm nextMonth = monthStart;
    nextMonth.tm_mon++;
    mktime(&nextMonth);
    nextMonth.tm_mday = 0;
    mktime(&nextMonth);
    int daysInMonth = nextMonth.tm_mday;
    
    /* Draw calendar days */
    int row = 0;
    for (int day = 1; day <= daysInMonth; day++) {
        struct tm dayTm = *t;
        dayTm.tm_mday = day;
        mktime(&dayTm);
        int dow = dayTm.tm_wday;
        
        if (dow == 0 && day > 1) row++;
        
        char numStr[4];
        snprintf(numStr, sizeof(numStr), "%d", day);
        
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
    
    /* Holiday section - moved down, use dynamic data */
    float holidayY = y + 180 + (row + 1) * 38 + 10;
    RenderText("Next Holiday:", x + 20, holidayY, font_small, &COLOR_CYAN);
    RenderText(g_holidayName, x + 20, holidayY + 22, font_small, &COLOR_WHITE);
    RenderText(g_holidayDays, x + 20, holidayY + 42, font_small, &COLOR_ORANGE);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Dashboard Clock", "1.0", "com.example.dashboard");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Dashboard", SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("TTF init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /* Load fonts */
    const char *fontPaths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf",
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

    /* Initialize curl */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    /* Fetch initial wallpaper */
    lastWallpaperFetch = 0;  /* Force immediate fetch */
    FetchBingWallpaper();    /* Background image */
    
    /* Fetch all data from web APIs */
    FetchWeatherData();    /* Guangzhou weather */
    FetchAQIData();        /* Guangzhou AQI */
    FetchHolidays();       /* Next holiday */
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    FetchLunarDate(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    GetSolarTerm(t->tm_mon + 1, t->tm_mday);

    SDL_HideCursor();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    /* Check if wallpaper needs update (every 5 minutes) */
    time_t now = time(NULL);
    if (now - lastWallpaperFetch >= WALLPAPER_UPDATE_INTERVAL) {
        FetchBingWallpaper();
        lastWallpaperFetch = now;
    }
    
    /* Draw Bing wallpaper background */
    if (bgTexture) {
        /* Clear with black first */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        /* Set texture color to full brightness */
        SDL_SetTextureColorMod(bgTexture, 255, 255, 255);
        SDL_SetTextureAlphaMod(bgTexture, 255);
        
        /* Draw texture scaled to full screen */
        SDL_FRect dstRect = {0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
        SDL_RenderTexture(renderer, bgTexture, NULL, &dstRect);
        
        /* Draw semi-transparent overlay for better text readability */
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 15, 30, 160);
        SDL_FRect overlay = {0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT};
        SDL_RenderFillRect(renderer, &overlay);
        
        SDL_Log("Background texture rendered");
    } else {
        /* Fallback: gradient background */
        SDL_SetRenderDrawColor(renderer, 20, 30, 60, 255);
        SDL_RenderClear(renderer);
        
        for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
            float ratio = (float)y / SCREEN_HEIGHT;
            SDL_Color c = {
                .r = (int)(40 * (1-ratio) + 15 * ratio),
                .g = (int)(50 * (1-ratio) + 25 * ratio),
                .b = (int)(90 * (1-ratio) + 50 * ratio),
                .a = 255
            };
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_FRect rect = {0, y, SCREEN_WIDTH, 4};
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    /* Main left panel */
    DrawCard(20, 80, 940, 600, &COLOR_PANEL_BG);
    
    /* Get current time */
    struct tm *t = localtime(&now);
    
    /* Temperature display - Guangzhou real weather */
    RenderText(g_tempStr, 50, 150, font_large, &COLOR_WHITE);
    
    /* Location */
    RenderText("Guangzhou/CN", 50, 250, font_medium, &COLOR_GRAY);
    
    /* Weather icon */
    DrawWeatherIcon(340, 170, 1.2f);
    
    /* Analog clock */
    DrawClock(670, 310, 140);
    
    /* Info cards row - moved right */
    DrawInfoCard(70, 530, "Humidity", g_humidityStr, g_humidityDesc, &COLOR_CARD_1);
    DrawInfoCard(240, 530, "Wind", g_windStr, g_windDir, &COLOR_CARD_2);
    DrawInfoCard(410, 530, "AQI", g_aqiStr, g_aqiDesc, &COLOR_CARD_3);
    DrawInfoCard(580, 530, "Pressure", g_pressureStr, g_pressureDesc, &COLOR_CARD_4);
    DrawInfoCard(750, 530, "Sunrise", g_sunriseStr, g_sunsetStr, &COLOR_CARD_5);
    
    /* Right calendar panel - 10px gap from left panel */
    DrawCalendar(970, 110);

    SDL_RenderPresent(renderer);
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (font_large) TTF_CloseFont(font_large);
    if (font_medium) TTF_CloseFont(font_medium);
    if (font_small) TTF_CloseFont(font_small);
    if (bgTexture) SDL_DestroyTexture(bgTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    curl_global_cleanup();
}
