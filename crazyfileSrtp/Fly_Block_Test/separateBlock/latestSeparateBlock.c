#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include<math.h>
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

/*宏变量声明*/

//广播任务的堆栈与优先级设置
#define RADIO_STACK_SIZE 1000
#define RADIO_MAIN_PRI 1

//增加邻居的线程等待时间
#define WAIT_TIME_MS 300

//信号量管理
SemaphoreHandle_t WriteBaseMutex;      // base_info写入互斥锁
    
SemaphoreHandle_t ReadBaseMutex;       // base_info读取互斥锁



/*结构体声明*/
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

//存储鸟的基本信息
struct Base_Info
{
    char id;
    int baseRotationSpeed;         //基础旋转角速度
    int currRotaltionSpeed;        //当前旋转角速度
    float baseSpeed;               //基础速度
    float currSpeed;               //当前速度
    struct Position position;      //鸟的当前位置
    struct Direction currDirection;    //当前运动方向
    struct Direction desiredDirection; //预期运动方向
};
//存储鸟根据实际环境变换导致存储大小变化的信息
struct Vari_Info
{
    struct Boid* nearbyBoidsArray;     //邻居链表
    int numNearbyArray;          //分离邻居的个数
    SemaphoreHandle_t neighborMutex;  // 互斥锁用于保护邻居链表
};
//鸟
struct Boid
{
  struct Base_Info base;
  struct Vari_Info vari;
};


//独立发送的参数列表，传递给单独的进程任务
struct TaskParameters {
    struct Base_Info * baseInfopacket;
    P2PPacket *p_reply;
};

//因为boid是整个逻辑都要使用且随时记录变化的，所以使用值传参的方法，保证整个系统是动态变化的
/*基本的信息操作*/


//更新位置
void updatePosition(struct Position* position)
{       
        DEBUG_PRINT("x的更新前坐标：%f\n",(double)position->x); 
        logVarId_t idX = logGetVarId("stateEstimate", "x");
        float X = 0.0f;
        X = logGetFloat(idX);
        position->x = (double)X;
        DEBUG_PRINT("x的更新坐标：%f\n",(double)position->x); 

        DEBUG_PRINT("y的新前坐标：%f\n",(double)position->y); 
        logVarId_t idY = logGetVarId("stateEstimate", "y");
        float Y = 0.0f;
        Y = logGetFloat(idY);
        position->y = (double)Y;
        DEBUG_PRINT("y的更新坐标：%f\n",(double)position->y);
        
        DEBUG_PRINT("z的更新前坐标：%f\n",(double)position->z); 
        logVarId_t idZ = logGetVarId("stateEstimate", "z");
        float Z = 0.0f;
        Z = logGetFloat(idZ);
        position->z = (double)Z;
        DEBUG_PRINT("z的更新坐标：%f\n",(double)position->z);
};
//方向初始化
void  InitDirection(struct Direction* direction)
{
   direction->x=0;
   direction->y=0;
   direction->z=0;
}

//基本信息初始化
void initBase(struct Base_Info* base)
{
  base->id='A'; 
  base->baseRotationSpeed = 170;
  base->currRotaltionSpeed =170;
  base->baseSpeed = 0.8;
  base->currSpeed = 0.8;
  InitDirection(&base->currDirection);
  InitDirection(&base->desiredDirection);
  updatePosition(&base->position);
};

//变化信息初始化
void initVariInfo(struct Vari_Info* vari)
{
     vari->nearbyBoidsArray=NULL;
     vari->numNearbyArray=0;
     vari->neighborMutex = xSemaphoreCreateMutex();
};

//鸟类个体初始化
void InitBoid(struct Boid *curboid)
{
     DEBUG_PRINT("鸟类初始化\n");
     initBase(&curboid->base);
     initVariInfo(&curboid->vari);

};



/*独立广播逻辑*/


// 在主任务中发送数据包
void sendDataPacketTask(void *pvParameters)
{
    struct TaskParameters *params = (struct TaskParameters *)pvParameters;
    struct Base_Info *infoForTask =params->baseInfopacket; 
    P2PPacket *p_reply = params->p_reply;
    while (1)
    {
        // 更新坐标
        if(xSemaphoreTake(WriteBaseMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("写锁此时是放开的，赶快锁\n");
        DEBUG_PRINT("测试有没有传进来，%c\n",infoForTask->id);
        updatePosition(&infoForTask->position);
        DEBUG_PRINT("重置数据包\n");
        xSemaphoreGive(WriteBaseMutex);
        DEBUG_PRINT("写完了，放开锁\n");
        };

        DEBUG_PRINT("数据变化\n");
        DEBUG_PRINT("变化的x%f\n",(double)infoForTask->position.x);
        DEBUG_PRINT("变化的x%f\n",(double)infoForTask->position.y);
        DEBUG_PRINT("变化的x%f\n",(double)infoForTask->position.z);

        
        
        memcpy(p_reply->data, infoForTask, sizeof(struct Base_Info));
        p_reply->size = sizeof(struct Base_Info);
        
        struct Base_Info testBase;
        memcpy(&testBase,&p_reply->data[0], sizeof(struct Base_Info));


        // DEBUG_PRINT("打印第二次测试的数据包\n");

        // DEBUG_PRINT("飞机的id是:%c\n",params->baseInfopacket->id);

        // DEBUG_PRINT("x坐标是%f\n",(double)infoForTask->position.x);
    
        // DEBUG_PRINT("y坐标是%f\n",(double)infoForTask->position.y);
    
        // DEBUG_PRINT("z坐标是%f\n",(double)infoForTask->position.z);  

        // 设置广播周期
        vTaskDelay(M2T(3000));
        radiolinkSendP2PPacketBroadcast(p_reply);
         
        
        // 继续执行其他任务或延时
        vTaskDelay(M2T(1000)); // 假设在发送数据包后等待1秒
    }
}




void p2pcallbackHandler(P2PPacket *p)
{
 //序列复制
    static struct Base_Info copyBase;
    memcpy(&copyBase,&p->data[0], sizeof(struct Base_Info));
    
    //数据呈现
    DEBUG_PRINT("打印接收的数据包\n");
    DEBUG_PRINT("飞机的id是:%c\n",copyBase.id);

    DEBUG_PRINT("x坐标是%f\n",(double)copyBase.position.x);
    
    DEBUG_PRINT("y坐标是%f\n",(double)copyBase.position.y);
    
    DEBUG_PRINT("z坐标是%f\n",(double)copyBase.position.z);
}

void appMain()
{
   DEBUG_PRINT("Waiting for activation ...\n");

    // Initialize the p2p packet 
    P2PPacket p_reply;
    p_reply.port=0x00;
    
     //信号量初始化
        //将两个锁都放开
    WriteBaseMutex = xSemaphoreCreateMutex();
    ReadBaseMutex  = xSemaphoreCreateMutex();
    

    //初始化并拷贝
    struct Boid boid;
    InitBoid(&boid);
     
    DEBUG_PRINT("测试初始化的鸟：\n");
    DEBUG_PRINT("测试初始化的鸟id：%c\n", boid.base.id);
    DEBUG_PRINT("测试初始化的鸟basespeed：%d\n", boid.base.baseRotationSpeed);
    DEBUG_PRINT("测试初始化的鸟currspeed：%d\n", boid.base.currRotaltionSpeed);
    DEBUG_PRINT("测试初始化的鸟basespeed：%f\n", (double)boid.base.baseSpeed);
    DEBUG_PRINT("测试初始化的鸟currspeed：%f\n", (double)boid.base.currSpeed);
    DEBUG_PRINT("测试初始化的鸟positon：%f\n", (double)boid.base.position.x);
    DEBUG_PRINT("测试初始化的鸟positon：%f\n", (double)boid.base.position.y);
    DEBUG_PRINT("测试初始化的鸟positon：%f\n", (double)boid.base.position.z);
    
    
    memcpy(&p_reply.data,&boid.base,sizeof(struct Base_Info));
     
    p_reply.size=sizeof(struct Base_Info)+1; 
    // Register the callback function so that the CF can receive packets as well.
    p2pRegisterCB(p2pcallbackHandler);
    
    //将整个广播体系独立起来，相当于鸟类的声波器官
    // 创建结构体并初始化参数
    struct TaskParameters params;
    params.baseInfopacket = &boid.base;//指向base的指针
    params.p_reply = &p_reply; // 指向 p_reply 的指针
    xTaskCreate(sendDataPacketTask, "send_data_packet_task", RADIO_STACK_SIZE, &params, RADIO_MAIN_PRI, NULL);
    
}


//播报独立，当前问题，freeRtos的值传参无法对oid进行修改
//邻居判断与添加，减少
//方向计算
//分离实现
