#include "network.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include <stdlib.h>

// 全局变量
static net_state_t net_status = NET_STATE_IDLE;
static uint32_t state_timeout = 0;
static uint8_t retry_count = 0;



/**
  * @brief  发布MQTT消息接口
  */
void MQTT_Publish(const char *topic, const char *message, uint8_t qos, uint8_t retain)
{
    char pub_cmd[256];
    snprintf(pub_cmd, sizeof(pub_cmd), "AT+MQTTPUB=0,\"%s\",\"%s\",%d,%d\r\n", 
             topic, message, qos, retain);
    UART2_SendString_Timeout(pub_cmd, strlen(pub_cmd), 100);
}


/**
  * @brief  清除环形缓冲区的已读数据
  */
void UART_Buf_ClearRead(uart_buf_type *buf)
{
    buf->rp = buf->wp; // 简单清除所有数据
}

char temp_buffer[1024] = {0};
/**
  * @brief  在环形缓冲区中查找字符串
  * @param  buf: 环形缓冲区指针
  * @param  str: 要查找的字符串
  * @retval 1:找到, 0:未找到
  */
uint8_t UART_Buf_FindString(uart_buf_type *buf, const char *str)
{

    if (buf == NULL || str == NULL || buf->rp == buf->wp) {
		return 0;
    }

    uint16_t data_len = UART_Buf_DataCount(buf);

	if (buf->rp + data_len <= 1024) {
		// 情况1：数据连续，没有跨越缓冲区边界
		memcpy(temp_buffer, &buf->buf[buf->rp], data_len);
	} else {
		// 情况2：数据跨越缓冲区边界，需要分两段拷贝
		uint16_t first_part = 1024 - buf->rp;
		uint16_t second_part = data_len - first_part;
		
		memcpy(temp_buffer, &buf->buf[buf->rp], first_part);
		memcpy(temp_buffer + first_part, &buf->buf[0], second_part);
	}

	if (strstr(temp_buffer, str) != NULL) {
		return 1;
	} else {
		return 0;
	}
}

/**
  * @brief  解析MQTT订阅接收的消息
  * @param  buffer: 接收到的原始字符串
  * @param  topic: 输出主题指针
  * @param  data: 输出数据指针
  * @param  data_len: 输出数据长度
  * @retval 1:解析成功, 0:解析失败
  */
uint8_t MQTT_ParseSubRecv(const char *buffer, char *topic, char *data, uint16_t *data_len)
{
    // 格式: +MQTTSUBRECV:0,"topic",length,data
    if (strstr(buffer, "+MQTTSUBRECV:") != buffer) {
        return 0;
    }
    
    const char *ptr = buffer + 13; // 跳过 "+MQTTSUBRECV:"
    
    // 解析LinkID (跳过)
	/*
	作用：在字符串 str 中查找字符',' 的第一次出现
	返回值：
	找到：返回指向第一次出现位置的指针
	未找到：返回 NULL
	*/
    ptr = strchr(ptr, ',');
    if (!ptr) return 0;
    ptr++; // 跳过逗号
    
    // 解析主题
    if (*ptr == '"') {
        ptr++; // 跳过引号
        const char *topic_end = strchr(ptr, '"');
        if (!topic_end) return 0;
        
        uint16_t topic_len = topic_end - ptr;
        strncpy(topic, ptr, topic_len);
        topic[topic_len] = '\0';
        
        ptr = topic_end + 1; // 跳过引号和逗号
    }
    
    // 解析数据长度
    ptr = strchr(ptr, ',');
    if (!ptr) return 0;
    ptr++; // 跳过逗号
    
    *data_len = atoi(ptr);
    
    // 解析数据
    ptr = strchr(ptr, ',');
    if (!ptr) return 0;
    ptr++; // 跳过逗号
    
    strncpy(data, ptr, *data_len);
    data[*data_len] = '\0';
    
    return 1;
}

void Handle_MQTT_Message(const char *raw_message)
{
    char topic[128];
    char data[256];
    uint16_t data_len;
    
    if (MQTT_ParseSubRecv(raw_message, topic, data, &data_len)) 
	{
        printf("Received MQTT message:\n");
        printf("  Topic: %s\n", topic);
        printf("  Data: %s\n", data);
        printf("  Length: %d\n", data_len);
    }
}


/**
  * @brief  监控MQTT连接状态
  * @retval 连接状态: 1=已连接, 0=已断开, -1=状态未知
  */
int8_t MQTT_CheckConnectionStatus(uart_buf_type *buf)
{
    // 连接成功
    if (UART_Buf_FindString(buf, "+MQTTCONNECTED:0,1")) {
        printf("MQTT: Connected to server\n");
        UART_Buf_ClearRead(buf);
        return 1;
    }
    
    // 连接断开 - 关键监控点
    if (UART_Buf_FindString(buf, "+MQTTCONNECTED:0,0")) {
        printf("MQTT: Connection closed by server\n");
        UART_Buf_ClearRead(buf);
        return 0;
    }
    
    // 连接断开 - 关键监控点
    if (UART_Buf_FindString(buf, "+MQTTDISCONNECTED:0")) {
        printf("MQTT: Disconnected from server\n");
        UART_Buf_ClearRead(buf);
        return 0;
    }
    
    // 网络异常断开
    if (UART_Buf_FindString(buf, "CLOSED")) {
        printf("MQTT: Network connection closed\n");
        UART_Buf_ClearRead(buf);
        return 0;
    }
    
    return -1; // 状态未知
}


/**
  * @brief  网络状态机任务
  */
void my_network_task(void)
{
    uint32_t current_tick = HAL_GetTick();
    switch(net_status)
    {
        case NET_STATE_IDLE:
            printf("Network: Start initialization...\r\n");
            UART_Buf_ClearRead(&uart2);
            retry_count = 0;
            net_status = NET_STATE_SEND_AT_RST;
            break;
        case NET_STATE_SEND_AT_RST:
			printf("Network: Sending AT+RST...\r\n");
			UART2_SendString_Timeout("AT+RST\r\n", strlen("AT+RST\r\n"), 100);
			net_status = NET_STATE_WAIT_AT_RST;
            state_timeout = current_tick;
		break;
		case NET_STATE_WAIT_AT_RST:
			if (UART_Buf_FindString(&uart2, "OK")) {
                printf("Network: AT+RST OK!\r\n");
                net_status = NET_STATE_SEND_AT;
                retry_count = 0;
				HAL_Delay(2000);
				UART_Buf_ClearRead(&uart2);
            }
            else if (UART_Buf_FindString(&uart2, "ERROR")) {
                printf("Network: AT+RST ERROR!\r\n");
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_SEND_AT_RST; // 立即重发
            }
            else if (current_tick - state_timeout >= 3000) {
                printf("Network: AT+RST timeout!\r\n");
                if (++retry_count >= 3) {
                    net_status = NET_STATE_ERROR;
                } else {
                    net_status = NET_STATE_SEND_AT_RST;
                }
            }
		break;	
        case NET_STATE_SEND_AT:
            printf("Network: Sending AT...\r\n");
            UART2_SendString_Timeout("AT\r\n", strlen("AT\r\n"), 100);
            net_status = NET_STATE_WAIT_AT;
            state_timeout = current_tick;
            break;
            
        case NET_STATE_WAIT_AT:
            if (UART_Buf_FindString(&uart2, "OK")) {
                printf("Network: AT OK!\r\n");
				HAL_Delay(300);
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_SEND_WIFI_MODE;
                retry_count = 0;
            }
            else if (UART_Buf_FindString(&uart2, "ERROR")) {
                printf("Network: AT ERROR!\r\n");
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_SEND_AT; // 立即重发
            }
            else if (current_tick - state_timeout >= 3000) {
                printf("Network: AT timeout!\r\n");
                if (++retry_count >= 3) {
                    net_status = NET_STATE_ERROR;
                } else {
                    net_status = NET_STATE_SEND_AT;
                }
            }
            break;
            
        case NET_STATE_SEND_WIFI_MODE:
            printf("Network: Setting WiFi mode...\r\n");
            UART2_SendString_Timeout("AT+CWMODE=1\r\n", strlen("AT+CWMODE=1\r\n"), 100);
            net_status = NET_STATE_WAIT_WIFI_MODE;
            state_timeout = current_tick;
            break;
            
        case NET_STATE_WAIT_WIFI_MODE:
            if (UART_Buf_FindString(&uart2, "OK")) {
                printf("Network: WiFi mode set!\r\n");
				HAL_Delay(300);
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_SEND_WIFI_CONN;
                retry_count = 0;
            }
            else if (UART_Buf_FindString(&uart2, "ERROR")) {
                printf("Network: WiFi mode error!\r\n");
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_ERROR;
            }
            else if (current_tick - state_timeout >= 3000) {
                printf("Network: WiFi mode timeout!\r\n");
                net_status = NET_STATE_ERROR;
            }
            break;
		case NET_STATE_SEND_WIFI_CONN:
            printf("Network: Connecting to WiFi...\r\n");
            char wifi_cmd[128];
            snprintf(wifi_cmd, sizeof(wifi_cmd), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
			printf("WIFI_CONN :%s\r\n",wifi_cmd);
			UART2_SendString_Timeout(wifi_cmd, strlen(wifi_cmd), 100);
            net_status = NET_STATE_WAIT_WIFI_CONN;
            state_timeout = current_tick;
            break;
            
        case NET_STATE_WAIT_WIFI_CONN:
            if (UART_Buf_FindString(&uart2, "WIFI GOT IP")) {
                printf("Network: WiFi connected!\r\n");
				HAL_Delay(300);
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_SEND_MQTT_CFG;
                retry_count = 0;
            }
            else if (UART_Buf_FindString(&uart2, "FAIL")) {
                printf("Network: WiFi connect failed!\r\n");
                UART_Buf_ClearRead(&uart2);
                if (++retry_count >= 3) {
                    net_status = NET_STATE_ERROR;
                } else {
                    net_status = NET_STATE_SEND_WIFI_CONN;
                }
            }
            else if (current_tick - state_timeout >= 15000) {
                printf("Network: WiFi connect timeout!\r\n");
                if (++retry_count >= 3) {
                    net_status = NET_STATE_ERROR;
                } else {
                    net_status = NET_STATE_SEND_WIFI_CONN;
                }
            }
            break;
            
        case NET_STATE_SEND_MQTT_CFG:
            printf("Network: Configuring MQTT...\r\n");
            char mqtt_cfg[256];
            snprintf(mqtt_cfg, sizeof(mqtt_cfg), 
                     "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
                     MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
			printf("mqtt_cfg:%s\r\n",mqtt_cfg);
            UART2_SendString_Timeout(mqtt_cfg, strlen(mqtt_cfg), 100);
            net_status = NET_STATE_WAIT_MQTT_CFG;
            state_timeout = current_tick;
            break;
            
        case NET_STATE_WAIT_MQTT_CFG:
            if (UART_Buf_FindString(&uart2, "OK")) {
                printf("Network: MQTT configured!\r\n");
				HAL_Delay(300);
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_SEND_MQTT_CONN_CFG;
                retry_count = 0;
            }
            else if (UART_Buf_FindString(&uart2, "ERROR")) {
                printf("Network: MQTT config error!\r\n");
				// 打印缓冲区内容看具体错误信息
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_ERROR;
            }
            else if (current_tick - state_timeout >= 5000) {
                printf("Network: MQTT config timeout!\r\n");
				retry_count++;
				net_status = NET_STATE_SEND_MQTT_CFG;
				if(retry_count >= 3)
				{
					retry_count = 0;
					net_status = NET_STATE_ERROR;
				}   
            }
            break;
		case NET_STATE_SEND_MQTT_CONN_CFG:
			printf("Network: Setting MQTT connection config...\r\n");
			
			char mqtt_conn_cfg[256];
			snprintf(mqtt_conn_cfg, sizeof(mqtt_conn_cfg), 
					 "AT+MQTTCONNCFG=0,%d,%d,\"%s\",\"%s\",%d,%d\r\n",
					 MQTT_KEEPALIVE_TIME,
					 MQTT_CLEAN_SESSION,
					 MQTT_LWT_TOPIC,
					 MQTT_LWT_MESSAGE,
					 MQTT_LWT_QOS,
					 MQTT_LWT_RETAIN);
			
			printf("MQTT Conn Config: %s", mqtt_conn_cfg);
			UART2_SendString_Timeout(mqtt_conn_cfg, strlen(mqtt_conn_cfg), 100);
			net_status = NET_STATE_WAIT_MQTT_CONN_CFG;
			state_timeout = current_tick;
		break;

		case NET_STATE_WAIT_MQTT_CONN_CFG:
			if (UART_Buf_FindString(&uart2, "OK")) {
				printf("Network: MQTT connection config set successfully!\r\n");
				printf("KeepAlive: %d seconds\r\n", MQTT_KEEPALIVE_TIME);
				printf("LWT Topic: %s\r\n", MQTT_LWT_TOPIC);
				printf("LWT Message: %s\r\n", MQTT_LWT_MESSAGE);
				HAL_Delay(300);
                UART_Buf_ClearRead(&uart2);
				net_status = NET_STATE_SEND_MQTT_CONN;
				retry_count = 0;
			}
			else if (UART_Buf_FindString(&uart2, "ERROR")) {
				printf("Network: MQTT connection config error!\r\n");
				UART_Buf_ClearRead(&uart2);
				net_status = NET_STATE_ERROR;
			}
			else if (current_tick - state_timeout >= 10000) {
				printf("Network: MQTT connection config timeout!\r\n");
				retry_count++;
				net_status = NET_STATE_SEND_MQTT_CONN_CFG;
				if(retry_count >= 3)
				{
					retry_count = 0;
					net_status = NET_STATE_ERROR;
				}
			}
		break;
	
        case NET_STATE_SEND_MQTT_CONN:
            printf("Network: Connecting to MQTT...\r\n");
            char mqtt_conn[128];
            snprintf(mqtt_conn, sizeof(mqtt_conn), 
                     "AT+MQTTCONN=0,\"%s\",%d,1\r\n", 
                     MQTT_BROKER, MQTT_PORT);
            UART2_SendString_Timeout(mqtt_conn, strlen(mqtt_conn), 100);
            net_status = NET_STATE_WAIT_MQTT_CONN;
            state_timeout = current_tick;
            break;
            
        case NET_STATE_WAIT_MQTT_CONN:
//			printf("ESP8266 rev: \r\n");
//			for(uint16_t i = 0; i < UART_Buf_DataCount(&uart2); i++) 
//			{
//				printf("%c",uart2.buf[uart2.rp+i]);
//			}
//			printf("\r\n");
            if (UART_Buf_FindString(&uart2, "+MQTTCONNECTED:0,1")) {
                printf("Network: MQTT connected!\r\n");
				HAL_Delay(300);
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_SEND_SUBSCRIBE;
                retry_count = 0;
            }
            else if (UART_Buf_FindString(&uart2, "ERROR")) {
                printf("Network: MQTT connect failed!\r\n");
                UART_Buf_ClearRead(&uart2);
                if (++retry_count >= 3) {
                    net_status = NET_STATE_ERROR;
                } else {
                    net_status = NET_STATE_SEND_MQTT_CONN;
                }
            }
            else if (current_tick - state_timeout >= 10000) {
                printf("Network: MQTT connect timeout!\r\n");
                if (++retry_count >= 3) {
                    net_status = NET_STATE_ERROR;
                } else {
                    net_status = NET_STATE_SEND_MQTT_CONN;
                }
            }
            break;
            
        case NET_STATE_SEND_SUBSCRIBE:
            printf("Network: Subscribing to topics...\r\n");
            UART2_SendString_Timeout("AT+MQTTSUB=0,\"fyz_8266/control\",1\r\n", 
                                   strlen("AT+MQTTSUB=0,\"fyz_8266/control\",1\r\n"), 100);
            net_status = NET_STATE_WAIT_SUBSCRIBE;
            state_timeout = current_tick;
            break;
            
        case NET_STATE_WAIT_SUBSCRIBE:
            if (UART_Buf_FindString(&uart2, "OK")) {
                printf("Network: Subscribe success!\r\n");
				MQTT_Publish(MQTT_LWT_TOPIC,MQTT_ONLINE_MESSAGE,1,1);
                UART_Buf_ClearRead(&uart2);
                net_status = NET_STATE_WAIT_ONLINE;
                printf("=== Network: All setup completed! ===\r\n");
            }
            else if (current_tick - state_timeout >= 5000) {
                printf("Network: Subscribe timeout!\r\n");
                net_status = NET_STATE_ERROR;
            }
            break;
        case NET_STATE_WAIT_ONLINE:
			if (UART_Buf_FindString(&uart2, "OK")) {
				UART_Buf_ClearRead(&uart2);
				net_status = NET_STATE_RUNNING;
				retry_count = 0;
			}
			else if (UART_Buf_FindString(&uart2, "ERROR")) {
				printf("Network: MQTT online error!\r\n");
				UART_Buf_ClearRead(&uart2);
				net_status = NET_STATE_ERROR;
			}
			else if (current_tick - state_timeout >= 10000) {
				printf("Network: MQTT online timeout!\r\n");
				net_status = NET_STATE_ERROR;
			}	
			break;
        case NET_STATE_RUNNING:
		    // 检查连接状态
			switch(MQTT_CheckConnectionStatus(&uart2)) 
			{
				case 1: // 已连接
					break;
				case 0: // 已断开
					printf("MQTT: Connection lost, reconnecting...\n");
					net_status = NET_STATE_SEND_MQTT_CONN;
					break;
			}
			// 处理接收到的消息
			if (UART_Buf_FindString(&uart2, "+MQTTSUBRECV:")) {
				// 提取完整消息
				uint16_t data_len = UART_Buf_DataCount(&uart2);
				char message[512];
				
				for(uint16_t i = 0; i < data_len; i++) {
					uint16_t index = (uart2.rp + i) % 1024;
					message[i] = uart2.buf[index];
				}
				message[data_len] = '\0';
				UART_Buf_ClearRead(&uart2);
				Handle_MQTT_Message(message);
			}
            break;
            
        case NET_STATE_ERROR:
            printf("Network: Error state, retry in 5s...\r\n");
            HAL_Delay(5000);
            net_status = NET_STATE_IDLE;
            break;
            
        default:
            net_status = NET_STATE_IDLE;
            break;
	}
}


