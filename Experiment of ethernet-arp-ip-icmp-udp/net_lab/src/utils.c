#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define IPTOSBUFFERS 12

/**
 * @brief ip转字符串
 * 
 * @param ip ip地址
 * @return char* 生成的字符串
 */
char *iptos(uint8_t *ip)
{
    static char output[IPTOSBUFFERS][3 * 4 + 3 + 1];
    static short which;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    sprintf(output[which], "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return output[which];
}

/**
 * @brief 初始化buffer为给定的长度，用于装载数据包
 * 
 * @param buf 要初始化的buffer
 * @param len 长度
 */
void buf_init(buf_t *buf, int len)
{
    buf->len = len;
    buf->data = buf->payload + BUF_MAX_LEN - len;
}

/**
 * @brief 为buffer在头部增加一段长度，用于添加协议头
 * 
 * @param buf 要修改的buffer
 * @param len 增加的长度
 */
void buf_add_header(buf_t *buf, int len)
{
    buf->len += len;
    buf->data -= len;
}

/**
 * @brief 为buffer在头部减少一段长度，去除协议头
 * 
 * @param buf 要修改的buffer
 * @param len 减少的长度
 */
void buf_remove_header(buf_t *buf, int len)
{
    buf->len -= len;
    buf->data += len;
}

/**
 * @brief 复制一个buffer到新buffer
 * 
 * @param dst 目的buffer
 * @param src 源buffer
 */
void buf_copy(buf_t *dst, buf_t *src)
{
    buf_init(dst, src->len);
    memcpy(dst->payload, src->payload, BUF_MAX_LEN);
}

/**
 * @brief 计算16位校验和
 *        1. 把首部看成以 16 位为单位的数字组成，依次进行二进制求和
 *           注意：求和时应将最高位的进位保存，所以加法应采用 32 位加法
 *        2. 将上述加法过程中产生的进位（最高位的进位）加到低 16 位
 *           采用 32 位加法时，即为将高 16 位与低 16 位相加，
 *           之后还要把该次加法最高位产生的进位加到低 16 位
 *        3. 将上述的和取反，即得到校验和。  
 *        
 * @param buf 要计算的数据包
 * @param len 要计算的长度
 * @return uint16_t 校验和
 */
uint16_t checksum16(uint8_t *buf, int len) //uint16_t 已改为 uint8_t
{
    // TODO
    uint32_t n32 = 0;
    uint16_t n16 = 0;
    for (int i = 0; i < len; i = i+2)
    {
        // if (i+1<len)
        // {
            n16 = buf[i]*256 + buf[i+1];
            n32 += n16;
        // }
        // else
        // {
        //     n16 = buf[i]*256;
        //     n32 += n16;
        // }
    }
    n16 = n32>>16;
    while (n16 != 0)
    {
        n32 = n32 & 0x0000ffff;
        n32 += n16;
        n16 = n32>>16;
    }    
    n16 = n32 & 0x0000ffff;
    n16 = ~n16;
    return n16;
}