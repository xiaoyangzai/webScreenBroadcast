#ifndef __WEBSOCKETPROTOCL_H__
#define __WEBSOCKETPROTOCL_H__

#include <stdint.h>

//是否结束数据包
#define IS_LASTPOCKET(code) (code & 0x80)

//获取数据包类型
#define OPCODE(code)		(code & 0x0F)

#define ADDITION_CODE		0x0
#define TEXT_CODE			0x1
#define BINARY_CODE			0x2
#define CLOSE_CODE			0x8
#define PING_CODE			0x9
#define PONG_CODE			0xA


#define sys_err(s) do{\
	fprintf(stderr,"[%s:%d] %s:%s\n",__FILE__,__LINE__,s,strerror(errno));\
	exit(-1);\
}while(0)

#define MAXLINE 1024

//base64加密算法
//@bindata: 需要加密的数据
//@base64: 加密后的数据存储缓冲区
//@binlength: 需要加密数据的长度
//返回值：返回加密后的数据存储缓冲区
char * base64_encode( const unsigned char * bindata, char * base64, int binlength );

//base64解密算法
//@base64: 需要解密的数据
//@bindata: 解密后的数据存储缓冲区
//返回值: 成功返回0
int base64_decode( const char * base64, unsigned char * bindata );

//响应websocket握手请求
//@clientfd: 与客户端通信的文件描述符
//@websocket_key: websocket握手协议中Sec-WebSocket-Key字段的值
void send_websocket_response(int clientfd,char *websocket_key);

//64位长整型网络字节序到本地字节序的转换
//若本地为大端存储模式，无序转换
//若本地为小端存储模式,则需要从大端到小端的转换
uint64_t ntohll(uint64_t nl);

//64位长整型本地字节序到网络字节序的转换
//若本地为大端存储模式，无序转换
//若本地为小端存储模式,则需要从大端到小端的转换
uint64_t htonll(uint64_t nl);

//是否数据经过掩码
#define IS_SETMASK(code)	(code & 0x80)

//处理WebSocket请求
int do_websocket(int clientfd,char *key);

//发送websocket协议包头部
//@clienfd: 与客户端通信的文件描述符
//@type:协议包中数据的类型 
//@len: 将要发送的数据长度
int send_websocket_header(int clientfd,uint8_t type,uint64_t len);

//SHA加密以及base64加密
//@key: Sec-WebSocket-key 
//@encode_key: 保存@key经过SHA和base64加密后的数据
int base64_sha_encode(const char* key, char* encode_key);

//获取数据长度
int get_websocket_data_len(uint8_t *frame_data,uint32_t *len);

//获取掩码值，并且存储到数组中
int get_websocket_maskKey(uint8_t *mask,uint8_t maskKey[]);

//解掩码或掩码
//转换后的结果存储在data中
int decode_websocke_data(uint8_t *data,uint32_t len,uint8_t maskKey[],uint8_t *outBuff); 

//get data
int get_websocket_data(uint8_t *data,uint8_t *outBuff);


//server  send data to client 
int send_websocket_data(int clientfd,uint8_t *data,uint64_t len,int flag);

#endif
