#ifndef __WEBSOCKETSERVER_H__
#define __WEBSOCKETSERVER_H__

typedef struct {
	//用于与客户端通信的文件描述符
	int clientfd;
	//客户端断开连接标志位
	int disconnect_flag;
	//线程互斥锁
	pthread_mutex_t mutex;
}global_resource;

//处理websocket请求
//建立websocket连接，发送屏幕图像数据到客户端，接收客户端断开数据
//@clienfd: 用于客户端通信的文件描述符
//@key: 客户端websocket握手请求包中的Sec-WebSocket-Key字段值
void do_websocket_response(int clientfd, char *key);

//接收客户端断开连接请求线程函数
void *pthread_recv(void *arg);

//向客户端发送屏幕图像数据线程函数
void *pthread_send(void *arg);
#endif
