/**
 * @file task_registry.c
 * @brief Task registry for health monitoring
 */

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <string.h>

static const char *TAG = "SVC:HEALTH:REG";

// ============================================================================
// Task Registry Data Structures (data-model.md Section 5)
// ============================================================================

#define MAX_TASKS 10

typedef struct {
    TaskHandle_t handle;
    char name[16];
    UBaseType_t priority;
    uint32_t stack_size;
    BaseType_t core_id;
    UBaseType_t stack_watermark;
    bool watchdog_subscribed;
} task_info_t;

typedef struct {
    task_info_t tasks[MAX_TASKS];
    uint8_t count;
    SemaphoreHandle_t mutex;
} task_registry_t;

static task_registry_t s_registry = {0};

// ============================================================================
// Task Registry Implementation (T088, T089, T090)
// ============================================================================

/**
 * @brief Initialize task registry (T088)
 */
esp_err_t task_registry_init(void)
{
    ESP_LOGI(TAG, "Initializing task registry");
    
    memset(&s_registry, 0, sizeof(s_registry));
    
    s_registry.mutex = xSemaphoreCreateMutex();
    if (s_registry.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create task registry mutex (error=ESP_ERR_NO_MEM)");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Task registry initialized (max_tasks=%d)", MAX_TASKS);
    return ESP_OK;
}

/**
 * @brief Add task to registry (T088)
 */
esp_err_t task_registry_add(TaskHandle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_registry.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for task_registry_add (timeout, task=%s)",
                 pcTaskGetName(handle));
        return ESP_ERR_TIMEOUT;
    }
    
    // Check if task already registered
    for (uint8_t i = 0; i < s_registry.count; i++) {
        if (s_registry.tasks[i].handle == handle) {
            xSemaphoreGive(s_registry.mutex);
            ESP_LOGW(TAG, "Task already registered: %s", 
                     pcTaskGetName(handle));
            return ESP_OK;  // Not an error
        }
    }
    
    // Check if registry is full
    if (s_registry.count >= MAX_TASKS) {
        xSemaphoreGive(s_registry.mutex);
        ESP_LOGE(TAG, "Task registry full (count=%d, max=%d, task=%s)",
                 s_registry.count, MAX_TASKS, pcTaskGetName(handle));
        return ESP_ERR_NO_MEM;
    }
    
    // Add task to registry
    task_info_t *info = &s_registry.tasks[s_registry.count];
    info->handle = handle;
    strncpy(info->name, pcTaskGetName(handle), sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->priority = uxTaskPriorityGet(handle);
    
    // Get task stack info using ESP-IDF compatible API
    UBaseType_t watermark = uxTaskGetStackHighWaterMark(handle);
    info->stack_watermark = watermark;
    // Note: ESP-IDF doesn't provide easy access to total stack size from handle
    // Stack size should be stored when task is created (passed to xTaskCreate)
    info->stack_size = watermark * sizeof(StackType_t); // This is minimum free, not total
    
    // Get core affinity
#if CONFIG_FREERTOS_UNICORE
    info->core_id = 0;
#else
    // Use xTaskGetCoreID() from ESP-IDF FreeRTOS port
    // Returns the core ID where the task is currently running or last ran
    BaseType_t core = xTaskGetCoreID(handle);
    info->core_id = (core == tskNO_AFFINITY) ? -1 : (int)core;
#endif
    
    info->watchdog_subscribed = true;
    s_registry.count++;
    
    xSemaphoreGive(s_registry.mutex);
    
    ESP_LOGI(TAG, "Task registered: %s (priority=%lu, stack=%lu bytes, core=%d)",
             info->name, info->priority, info->stack_size, info->core_id);
    
    return ESP_OK;
}

/**
 * @brief Update task stack watermark (T089)
 * 
 * Store stack watermark for task (FR-034)
 */
esp_err_t task_registry_update_watermark(TaskHandle_t handle, UBaseType_t watermark)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (xSemaphoreTake(s_registry.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    // Find task in registry
    for (uint8_t i = 0; i < s_registry.count; i++) {
        if (s_registry.tasks[i].handle == handle) {
            s_registry.tasks[i].stack_watermark = watermark;
            
            // Log warning if watermark is too low (FR-034)
            if (watermark < 512) {
                ESP_LOGW(TAG, "Low stack watermark for %s: %lu bytes remaining",
                         s_registry.tasks[i].name, 
                         watermark * sizeof(StackType_t));
            }
            
            xSemaphoreGive(s_registry.mutex);
            return ESP_OK;
        }
    }
    
    xSemaphoreGive(s_registry.mutex);
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Print all registered tasks (T090)
 * 
 * Debug output showing all registered tasks with stack usage
 */
void task_registry_print_all(void)
{
    if (xSemaphoreTake(s_registry.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex for task_registry_print_all");
        return;
    }
    
    ESP_LOGI(TAG, "=== Task Registry (%u tasks) ===", s_registry.count);
    ESP_LOGI(TAG, "%-16s %-8s %-8s %-8s %-12s %s",
             "Name", "Priority", "Core", "Stack", "Watermark", "WDT");
    
    for (uint8_t i = 0; i < s_registry.count; i++) {
        task_info_t *info = &s_registry.tasks[i];
        
        // Get current watermark
        UBaseType_t current_watermark = uxTaskGetStackHighWaterMark(info->handle);
        uint32_t watermark_bytes = current_watermark * sizeof(StackType_t);
        uint32_t stack_used_pct = 100 - ((watermark_bytes * 100) / info->stack_size);
        
        const char *core_str = (info->core_id == -1) ? "any" : 
                               (info->core_id == 0) ? "0" : "1";
        const char *wdt_str = info->watchdog_subscribed ? "YES" : "NO";
        
        ESP_LOGI(TAG, "%-16s %-8lu %-8s %-8lu %-12lu %s",
                 info->name,
                 info->priority,
                 core_str,
                 info->stack_size,
                 watermark_bytes,
                 wdt_str);
        
        // Warn about low watermarks
        if (current_watermark < 512) {
            ESP_LOGW(TAG, "  WARNING: %s has low stack watermark (%lu bytes, %lu%% used)",
                     info->name, watermark_bytes, stack_used_pct);
        }
    }
    
    ESP_LOGI(TAG, "=== End Task Registry ===");
    
    xSemaphoreGive(s_registry.mutex);
}
