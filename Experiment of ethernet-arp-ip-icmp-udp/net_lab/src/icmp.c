#include "icmp.h"
#include "ip.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 处理一个收到的数据包
 *        你首先要检查ICMP报头长度是否小于icmp头部长度
 *        接着，查看该报文的ICMP类型是否为回显请求，
 *        如果是，则回送一个回显应答（ping应答），需要自行封装应答包。
 * 
 *        应答包封装如下：
 *        首先调用buf_init()函数初始化txbuf，然后封装报头和数据，
 *        数据部分可以拷贝来自接收到的回显请求报文中的数据。
 *        最后将封装好的ICMP报文发送到IP层。  
 * 
 * @param buf 要处理的数据包
 * @param src_ip 源ip地址
 */
void icmp_in(buf_t *buf, uint8_t *src_ip)
{
    // TODO
    uint8_t *p = buf->data;
    uint16_t checksum;
    if (buf->len >= 8)
    {
        if (p[0] == 8 && p[1] == 0) 
        {
            buf_init(&txbuf, buf->len);
            buf_copy(&txbuf, buf);
            for (int i=0; i<4; i++) { txbuf.data[i] = 0; }
            checksum = checksum16(txbuf.data, buf->len);
            txbuf.data[2] = checksum >> 8;
            txbuf.data[3] = checksum & 0x00ff;
            ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);
        }
    }
}

/**
 * @brief 发送icmp不可达
 *        你需要首先调用buf_init初始化buf，长度为ICMP头部 + IP头部 + 原始IP数据报中的前8字节 
 *        填写ICMP报头首部，类型值为目的不可达
 *        填写校验和
 *        将封装好的ICMP数据报发送到IP层。
 * 
 * @param recv_buf 收到的ip数据包
 * @param src_ip 源ip地址
 * @param code icmp code，协议不可达或端口不可达
 */
void icmp_unreachable(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code)
{
    // TODO
    buf_t buf;
    uint16_t checksum;
    buf_init(&buf, 36);
    uint8_t *p = buf.data;
    uint8_t *q = recv_buf->data;
    p[0] = ICMP_TYPE_UNREACH;
    p[1] = code;
    for (int i = 0; i<6; i++) { p[2+i] = 0; }
    for (int i = 0; i<28; i++) { p[8+i] = q[i]; }
    checksum = checksum16(p, 36);
    p[2] = checksum >> 8;
    p[3] = checksum & 0x00ff;    
    ip_out(&buf, src_ip, NET_PROTOCOL_ICMP);
}