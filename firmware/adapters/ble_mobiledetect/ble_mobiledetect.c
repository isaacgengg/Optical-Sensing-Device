// BLE result sink over NimBLE. This is the only file in the project that
// includes NimBLE headers.

#include <stdbool.h>
#include <string.h>
#include "sdkconfig.h"
#include "ble_mobiledetect.h"
#include "mobiledetect_protocol.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

static const char *TAG = "ble_md";

// TODO(team): replace with MobileDetect's UUIDs.
// Random placeholders (bytes are little-endian, as NimBLE expects):
//   service        c0a5f1d4-5322-444a-8b52-678d10103cfc
//   characteristic 5407ab56-5c25-47f7-8ce3-71a01d48ad11
static const ble_uuid128_t SVC_UUID = BLE_UUID128_INIT(
    0xfc, 0x3c, 0x10, 0x10, 0x8d, 0x67, 0x52, 0x8b,
    0x4a, 0x44, 0x22, 0x53, 0xd4, 0xf1, 0xa5, 0xc0);
static const ble_uuid128_t CHR_UUID = BLE_UUID128_INIT(
    0x11, 0xad, 0x48, 0x1d, 0xa0, 0x71, 0xe3, 0x8c,
    0xf7, 0x47, 0x25, 0x5c, 0x56, 0xab, 0x07, 0x54);

// ponytail: plain volatiles shared between the NimBLE host task and the
// measurement task. Fine for one client; add a mutex if state grows.
static volatile uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static volatile bool s_notify_enabled;
static uint16_t s_chr_val_handle;
static uint8_t s_own_addr_type;
static uint8_t s_last[MDP_RESULT_V0_LEN];
static size_t s_last_len;
static result_sink_t s_sink;

static void advertise(void);

static int chr_access(uint16_t conn_handle, uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    return os_mbuf_append(ctxt->om, s_last, s_last_len) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static const struct ble_gatt_svc_def s_services[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &SVC_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                .uuid = &CHR_UUID.u,
                .access_cb = chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_chr_val_handle,
            },
            {0},
        },
    },
    {0},
};

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "connected");
            s_conn_handle = event->connect.conn_handle;
        } else {
            advertise();
        }
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnected, reason=%d", event->disconnect.reason);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        s_notify_enabled = false;
        advertise();
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == s_chr_val_handle) {
            s_notify_enabled = event->subscribe.cur_notify;
        }
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        advertise();
        break;
    default:
        break;
    }
    return 0;
}

static void advertise(void)
{
    const char *name = ble_svc_gap_device_name();
    struct ble_hs_adv_fields fields = {
        .flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,
        .name = (const uint8_t *)name,
        .name_len = strlen(name),
        .name_is_complete = 1,
    };
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGW(TAG, "adv_set_fields failed: %d", rc);
        return;
    }

    struct ble_gap_adv_params params = {
        .conn_mode = BLE_GAP_CONN_MODE_UND,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
    };
    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER, &params, gap_event, NULL);
    if (rc != 0) {
        ESP_LOGW(TAG, "adv_start failed: %d", rc);
    }
}

static void on_sync(void)
{
    if (ble_hs_util_ensure_addr(0) != 0 || ble_hs_id_infer_auto(0, &s_own_addr_type) != 0) {
        ESP_LOGW(TAG, "no usable BLE address");
        return;
    }
    advertise();
}

static void on_reset(int reason)
{
    ESP_LOGW(TAG, "host reset, reason=%d", reason);
}

static void host_task(void *param)
{
    (void)param;
    nimble_port_run();  // returns only when nimble_port_stop() is called
    nimble_port_freertos_deinit();
}

static esp_err_t ble_publish(void *ctx, const strip_result_t *result)
{
    (void)ctx;
    if (result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t len = mdp_encode_result(result, s_last, sizeof(s_last));
    if (len == 0) {
        return ESP_FAIL;
    }
    s_last_len = len;

    uint16_t conn = s_conn_handle;
    if (conn == BLE_HS_CONN_HANDLE_NONE || !s_notify_enabled) {
        return ESP_ERR_INVALID_STATE;
    }

    struct os_mbuf *om = ble_hs_mbuf_from_flat(s_last, len);
    if (om == NULL) {
        return ESP_ERR_NO_MEM;
    }
    int rc = ble_gatts_notify_custom(conn, s_chr_val_handle, om);
    if (rc != 0) {
        ESP_LOGW(TAG, "notify failed: %d", rc);
        return ESP_FAIL;
    }
    return ESP_OK;
}

result_sink_t *ble_mobiledetect_create(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs init failed: %s", esp_err_to_name(err));
        return NULL;
    }

    err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nimble init failed: %s", esp_err_to_name(err));
        return NULL;
    }

    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    if (ble_gatts_count_cfg(s_services) != 0 || ble_gatts_add_svcs(s_services) != 0) {
        ESP_LOGE(TAG, "GATT service registration failed");
        return NULL;
    }
    ble_svc_gap_device_name_set(CONFIG_DC_BLE_DEVICE_NAME);

    nimble_port_freertos_init(host_task);

    s_sink = (result_sink_t){.publish = ble_publish, .ctx = NULL};
    return &s_sink;
}
