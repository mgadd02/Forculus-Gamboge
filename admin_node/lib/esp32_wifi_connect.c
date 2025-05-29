#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/wifi_mgmt.h>
 
int start_wifi(void);
 
LOG_MODULE_REGISTER(MAIN);
 
#define WIFI_SSID "Nothing Phone (2a)_6061"     /* Replace `SSID` with WiFi ssid. */
#define WIFI_PSK  "Myphonerocks1"               /* Replace `PASSWORD` with Router password. */
 
#define NET_EVENT_WIFI_MASK \
    (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)
 
static struct net_if *sta_iface;
static struct wifi_connect_req_params sta_config;
static struct net_mgmt_event_callback cb;

/**
 * Event call backs for wifi connection and disconnet.
 */
static void wifi_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
                   struct net_if *iface)
{
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        LOG_INF("Connected to %s", WIFI_SSID);
        break;
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        LOG_INF("Disconnected from %s", WIFI_SSID);
        break;
    default:
        break;
    }
}

/**
 * Initialise connection to specified wifi in WIFI_SSID and WIFI_PSK
 */
static int connect_to_wifi(void) {

    if (!sta_iface) {
        LOG_ERR("STA: interface not initialized");
        return -EIO;
    }
 
    sta_config.ssid = (const uint8_t *)WIFI_SSID;
    sta_config.ssid_length = strlen(WIFI_SSID);
    sta_config.psk = (const uint8_t *)WIFI_PSK;
    sta_config.psk_length = strlen(WIFI_PSK);
    sta_config.security = WIFI_SECURITY_TYPE_PSK;
    sta_config.channel = WIFI_CHANNEL_ANY;
    sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;
 
    LOG_INF("Connecting to SSID: %s", WIFI_SSID);
 
    int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
               sizeof(sta_config));
    if (ret) {
        LOG_ERR("Unable to connect to %s (err: %d)", WIFI_SSID, ret);
    }
 
    return ret;
}

/**
 * Start wifi initialises callbacks and initialises connection to specified wifi  
 */ 
int start_wifi(void) {

    k_sleep(K_SECONDS(5));

    net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
    net_mgmt_add_event_callback(&cb);
 
    sta_iface = net_if_get_wifi_sta();
 
    return connect_to_wifi();
}
 