#ifndef __NET_WORK_H__
#define __NET_WORK_H__

#include "main.h"

// 网络状态定义
typedef enum {
    NET_STATE_IDLE = 0,        		// 空闲状态
	NET_STATE_SEND_AT_RST,	   		// 发送RESET指令
    NET_STATE_WAIT_AT_RST,     		// 等待RESE响应
    NET_STATE_SEND_AT,         		// 发送AT指令
    NET_STATE_WAIT_AT,         		// 等待AT响应
    NET_STATE_SEND_WIFI_MODE,  		// 发送WiFi模式设置
    NET_STATE_WAIT_WIFI_MODE,  		// 等待WiFi模式响应
    NET_STATE_SEND_WIFI_CONN,  		// 发送WiFi连接
    NET_STATE_WAIT_WIFI_CONN,  		// 等待WiFi连接响应
    NET_STATE_SEND_MQTT_CFG,   		// 发送MQTT配置
    NET_STATE_WAIT_MQTT_CFG,   		// 等待MQTT配置响应
	NET_STATE_SEND_MQTT_CONN_CFG, 	//设置连接参数（包括心跳）
	NET_STATE_WAIT_MQTT_CONN_CFG, 	//等待连接参数响应
    NET_STATE_SEND_MQTT_CONN,  		// 发送MQTT连接
    NET_STATE_WAIT_MQTT_CONN,  		// 等待MQTT连接响应
    NET_STATE_SEND_SUBSCRIBE,  		// 发送订阅
    NET_STATE_WAIT_SUBSCRIBE,  		// 等待订阅响应
	NET_STATE_WAIT_ONLINE,  		// 等待上线PUB回包
    NET_STATE_RUNNING,         		// 运行状态
    NET_STATE_ERROR            		// 错误状态
} net_state_t;

#define WIFI_SSID          "TPP"
#define WIFI_PASSWORD      "td88888888"

#define MQTT_BROKER        "app-management-server.washer-saas.istarix.com"
#define MQTT_PORT          20118
#define MQTT_CLIENT_ID     "saas_mu_999"
#define MQTT_USERNAME      "washer_saas_mu"
#define MQTT_PASSWORD      "$5ywq8bye5e7ah2hb*"


// MQTT连接配置
#define MQTT_KEEPALIVE_TIME    30      // 心跳时间(秒)
#define MQTT_CLEAN_SESSION     0       // 清理会话标志
#define MQTT_LWT_TOPIC         "fyz_8266/status"    // 遗嘱主题
#define MQTT_LWT_MESSAGE       "offline"           // 遗嘱消息
#define MQTT_LWT_QOS           1                   // 遗嘱QoS
#define MQTT_LWT_RETAIN        1                   // 遗嘱保留标志

#define MQTT_ONLINE_MESSAGE    "online"            // 上线消息

void my_network_task(void);
#endif

