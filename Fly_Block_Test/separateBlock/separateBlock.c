#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "app.h"

#include "FreeRTOS.h"
#include "task.h"
#include "system.h"
#include "semphr.h"
#include "radiolink.h"
#include "configblock.h"
#include "log.h"
#include "commander.h"
#include "stabilizer_types.h"
#include "param.h"
#define DEBUG_MODULE "P2P"
#include "debug.h"


//设置位置信息结构
struct Position
{
    float x;
    float y;
    float z;
    
};

//设置飞行方向
struct Direction
{
    float x;
    float y;
    float z;
    
};

//设置鸟类个体
struct Boid
{
    char id;
    struct Position position;
    struct Direction direction;
};


//定义飞行逻辑部分

//收集邻居的信息
void getAvoidBoids(struct Boid test){


};


//设置飞控部分，用于简单的收发飞控测试
//设置悬浮逻辑
static void setHoverSetpoint(setpoint_t *setpoint, float vx, float vy, float z, float yawrate)
{
  setpoint->mode.z = modeAbs;
  setpoint->position.z = z;
  setpoint->mode.yaw = modeVelocity;
  setpoint->attitudeRate.yaw = yawrate;
  setpoint->mode.x = modeVelocity;
  setpoint->mode.y = modeVelocity;
  setpoint->velocity.x = vx;
  setpoint->velocity.y = vy;
  setpoint->velocity_body = true;
  commanderSetSetpoint(setpoint, 3);
}
//起飞
void take_off()
{
  for (int i = 0; i < 100; i++)
  {
    setHoverSetpoint(&setpoint, 0, 0, height, 0);
    vTaskDelay(M2T(10));
  }
}
//降落
void land()
{
  int i = 0;
  float per_land = 0.05;
  while (height - i * per_land >= 0.05f)
  {
    i++;
    setHoverSetpoint(&setpoint, 0, 0, height - (float)i * per_land, 0);
    vTaskDelay(M2T(10));
  }
}
//测试案例，设计第一架悬浮高度超过0.5m后第二架飞机起飞

void testTask()
{

}



void p2pcallbackHandler(P2PPacket *p)
{

    // 输出数据包的内容
    DEBUG_PRINT("接收数据包中(Hexadecimal):\n");
    for (size_t i = 0; i < sizeof(struct Boid); i++) {
        DEBUG_PRINT("%02X \n ", (unsigned char)p->data[i]);
    }
    DEBUG_PRINT("接收结束(Hexadecimal):\n");
    
    
    struct Boid copyBoid;
    memcpy(&copyBoid,&p->data[0], sizeof(struct Boid));
    
    DEBUG_PRINT("打印接收的数据包\n");
    DEBUG_PRINT("COPY_ID: %c\n", copyBoid.id);
    DEBUG_PRINT("COPY_Position_x: %f\n", (double)copyBoid.position.x);
    DEBUG_PRINT("COPY_Position_y: %f\n", (double)copyBoid.position.y);
    DEBUG_PRINT("COPY_Position_z: %f\n", (double)copyBoid.position.z);
}

void appMain()
{   
    //存储分离邻居的列表
    //static struct Boid *separateBoids;

    //数据包初始化与端口设置
    static P2PPacket p_reply;
    p_reply.port = 0x00;
    

    while (1)
    {  
    //初始化鸟类个体并获取位置信息
    DEBUG_PRINT("初始化鸟类个体 \n");
    struct Boid boid;

    //计算Boid所需的char数组大小
    //size_t boidSize = sizeof(struct Boid);
    
    boid.id='C';

    DEBUG_PRINT("获取坐标中....\n");
    logVarId_t idX = logGetVarId("stateEstimate", "x");
    float X = 0.0f;
    X = logGetFloat(idX);
    boid.position.x = (double)X;
    
    logVarId_t idY = logGetVarId("stateEstimate", "y");
    float Y = 0.0f;
    Y = logGetFloat(idY);
    boid.position.y = (double)Y;
    
    logVarId_t idZ = logGetVarId("stateEstimate", "z");
    float Z = 0.0f;
    Z = logGetFloat(idZ);
    boid.position.z = (double)Z;
    
    DEBUG_PRINT("个体方向初始化...");
    boid.direction.x=0.0f;
    boid.direction.y=0.0f;
    boid.direction.z=0.0f;



    // 将结构体boid序列化到数据包的char数组
    memcpy(&p_reply.data, &boid, sizeof(struct Boid));
    p_reply.size=sizeof(struct Boid); 
    
    //注册回调函数
    p2pRegisterCB(p2pcallbackHandler);

    //设置广播周期
    vTaskDelay(M2T(5000));
    radiolinkSendP2PPacketBroadcast(&p_reply);
    };

}