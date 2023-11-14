//这一段开始用于测试设置在全局数据进行测试


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
#define RADIO_MAIN_PRI 2

//邻居类别划分的堆栈和优先级
#define DIVIDE_STACK_SIZE 2000
#define DIVIDE_MAIN_PRI 1


//增加邻居的线程等待时间
#define WAIT_TIME_MS 300






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
    struct Base_Info nearbyBoidsArray[10];     //总邻居测试数组
    struct Base_Info AvoidBoidsArray[10];      //分离数组
    int avoidNumber;
    struct Base_Info allignmentBoidsArray[10]; //对齐数组
    int allignmentNumber;
    struct Base_Info cohesionBoidsArray[10];   //聚集数组
    int cohesionNumber;

    SemaphoreHandle_t neighborMutex;   //基础的信息锁
};
//鸟
struct Boid
{
  struct Base_Info base;
  SemaphoreHandle_t BaseInfoMutex;  // 互斥锁用于保护整个的变量信息
  struct Vari_Info vari;
};

/*基本的信息操作*/

/*对于结构体的方法声明*/
//更新位置
void updatePosition(struct Position* position)
{       
        // DEBUG_PRINT("x的更新前坐标：%f\n",(double)position->x); 
        logVarId_t idX = logGetVarId("stateEstimate", "x");
        float X = 0.0f;
        X = logGetFloat(idX);
        position->x = (double)X;
        // DEBUG_PRINT("x的更新坐标：%f\n",(double)position->x); 

        // DEBUG_PRINT("y的新前坐标：%f\n",(double)position->y); 
        logVarId_t idY = logGetVarId("stateEstimate", "y");
        float Y = 0.0f;
        Y = logGetFloat(idY);
        position->y = (double)Y;
        // DEBUG_PRINT("y的更新坐标：%f\n",(double)position->y);
        
        // DEBUG_PRINT("z的更新前坐标：%f\n",(double)position->z); 
        logVarId_t idZ = logGetVarId("stateEstimate", "z");
        float Z = 0.0f;
        Z = logGetFloat(idZ);
        position->z = (double)Z;
        // DEBUG_PRINT("z的更新坐标：%f\n",(double)position->z);
};
//方向初始化
void  InitDirection(struct Direction* direction)
{
   direction->x=0;
   direction->y=0;
   direction->z=0;
}

//空信息初始化
void initBaseNull(struct Base_Info* base)
{
  base->id='W'; 
  base->baseRotationSpeed = 170;
  base->currRotaltionSpeed =170;
  base->baseSpeed = 0.8;
  base->currSpeed = 0.8;
  InitDirection(&base->currDirection);
  InitDirection(&base->desiredDirection);
  base->position.x=0;
  base->position.y=0;
  base->position.z=0;
};

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

    //对所有的初始空数组进行简单初始化，保证能够分配到对应的空间
    for(int i=0;i<5;i++){
        initBaseNull(&vari->nearbyBoidsArray[i]);
        
        initBaseNull(&vari->AvoidBoidsArray[i]);

        initBaseNull(&vari->cohesionBoidsArray[i]);

        initBaseNull(&vari->allignmentBoidsArray[i]);
    } 
    
    vari->neighborMutex = xSemaphoreCreateMutex();
};

//鸟类个体初始化
void InitBoid(struct Boid *curboid)
{
    //  DEBUG_PRINT("鸟类初始化\n");
     initBase(&curboid->base);
     initVariInfo(&curboid->vari);
     curboid->BaseInfoMutex = xSemaphoreCreateMutex();
};




/*这里是全局的实体变量声明*/

struct Boid boid;      //在全局的声明保证所有进程能够访问更新
P2PPacket p_reply;     //全局声明数据包





/*功能函数*/

//计算两个个体之间的距离
double calculateDistance(struct Position pos1, struct Position pos2)
{
    float deltaX = pos1.x - pos2.x;
    float deltaY = pos1.y - pos2.y;
    float deltaZ = pos1.z - pos2.z;

    // 使用欧几里得距离公式计算距离
    double distance = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);

    return distance;
}


//广播任务
void sendDataPacketTask(void *pvParameters)
{
    while (1)
    {   //去除信号量的更新
        // DEBUG_PRINT("写锁此时是放开的，赶快锁\n");
        // DEBUG_PRINT("测试有没有传进来，%c\n",boid.base.id);
        // updatePosition(&boid.base.position);
        // DEBUG_PRINT("重置数据包\n");
        // DEBUG_PRINT("写完了，放开锁\n");
        
      





        // 更新坐标
        if(xSemaphoreTake(boid.BaseInfoMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        // DEBUG_PRINT("写锁此时是放开的，赶快锁\n");
        // DEBUG_PRINT("测试有没有传进来，%c\n",boid.base.id);
        updatePosition(&boid.base.position);
        // DEBUG_PRINT("重置数据包\n");
        // DEBUG_PRINT("写完了，放开锁\n");
        xSemaphoreGive(boid.BaseInfoMutex);
        };

        // DEBUG_PRINT("数据变化\n");
        // DEBUG_PRINT("变化的x%f\n",(double)boid.base.position.x);
        // DEBUG_PRINT("变化的x%f\n",(double)boid.base.position.y);
        // DEBUG_PRINT("变化的x%f\n",(double)boid.base.position.z);

        
        
        memcpy(&p_reply.data, &boid.base, sizeof(struct Base_Info));
        p_reply.size = sizeof(struct Base_Info);
        
        struct Base_Info testBase;
        memcpy(&testBase,&p_reply.data[0], sizeof(struct Base_Info));


        // DEBUG_PRINT("打印第二次测试的数据包\n");

        // DEBUG_PRINT("飞机的id是:%c\n",params->baseInfopacket->id);

        // DEBUG_PRINT("x坐标是%f\n",(double)infoForTask->position.x);
    
        // DEBUG_PRINT("y坐标是%f\n",(double)infoForTask->position.y);
    
        // DEBUG_PRINT("z坐标是%f\n",(double)infoForTask->position.z);  

        // 设置广播周期
        vTaskDelay(M2T(3000));
        radiolinkSendP2PPacketBroadcast(&p_reply);
         
        
        // 继续执行其他任务或延时
        vTaskDelay(M2T(1000)); // 假设在发送数据包后等待1秒
    }
}

//添加分类，将飞机实验加入总的链表中，先以十架,在总列表中设置对应关系，A序号为0，B序号为1，以此类推。

void updateBoidQueue(struct Base_Info receiveBoid){
     switch(receiveBoid.id){
        case 'A':
        DEBUG_PRINT("收到A飞机的数据包，准备存入0号位置");
        if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[0]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
        break;

        case 'B':
        DEBUG_PRINT("收到B飞机的数据包，准备存入1号位置");
        if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[1]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;
        case 'C':
        DEBUG_PRINT("收到C飞机的数据包，准备存入2号位置");
         if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[2]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;
        case 'D':
        DEBUG_PRINT("收到D飞机的数据包，准备存入3号位置");
         if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[3]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;

        case 'E':
        DEBUG_PRINT("收到E飞机的数据包，准备存入4号位置");
         if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[4]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;
        case 'F':
        DEBUG_PRINT("收到F飞机的数据包，准备存入5号位置");
        if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[5]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
        
        break;
        case 'G':
        DEBUG_PRINT("收到G飞机的数据包，准备存入6号位置");
        if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[6]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;
        case 'H':
        DEBUG_PRINT("收到H飞机的数据包，准备存入7号位置");
        if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[7]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;
        case 'I':
        DEBUG_PRINT("收到i飞机的数据包，准备存入8号位置");
        if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[8]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;
        case 'J':
        DEBUG_PRINT("收到j飞机的数据包，准备存入9号位置");
        if(xSemaphoreTake(boid.vari.neighborMutex, pdMS_TO_TICKS(WAIT_TIME_MS)) == pdTRUE){
        DEBUG_PRINT("获取vari锁\n");
        boid.vari.nearbyBoidsArray[9]=receiveBoid;
        xSemaphoreGive(boid.vari.neighborMutex);
        DEBUG_PRINT("列表更新成功，释放锁。\n");
        };
       
        break;
        default:
        DEBUG_PRINT("未知错误，但是进入了程序");
        break;
     }
}

//将总的nearbyBoids列表划分进入三原则对应的列表

void divideBoidQueue(){
     while(1){    
     for(int i =0;i<10;i++){
        //计算每个邻居与当前鸟类的距离
        double dist = calculateDistance(boid.base.position,boid.vari.nearbyBoidsArray[i].position);
        // double dist = double(distance);
        if(0<dist&&dist<=0.6){
            boid.vari.AvoidBoidsArray[boid.vari.avoidNumber] = boid.vari.nearbyBoidsArray[i];
            boid.vari.avoidNumber++;
        }
        if(0.6<dist&&dist<=1){
            boid.vari.allignmentBoidsArray[boid.vari.allignmentNumber] = boid.vari.nearbyBoidsArray[i];
            boid.vari.allignmentNumber++;
        }
        if(1<dist&&dist<=2){
            boid.vari.cohesionBoidsArray[boid.vari.cohesionNumber] = boid.vari.nearbyBoidsArray[i];
            boid.vari.cohesionNumber++;
        }
    }
     boid.vari.avoidNumber=0;
     boid.vari.allignmentNumber=0;
     boid.vari.cohesionNumber=0;

     //测试部分

    DEBUG_PRINT("这里是本次收集到的邻居信息");
    DEBUG_PRINT("测试分离邻居的鸟id：%c\n", boid.vari.AvoidBoidsArray[0].id);
    DEBUG_PRINT("测试分离邻居的鸟basespeed：%d\n", boid.vari.AvoidBoidsArray[0].baseRotationSpeed);
    DEBUG_PRINT("测试分离邻居的鸟currspeed：%d\n", boid.vari.AvoidBoidsArray[0].currRotaltionSpeed);
    DEBUG_PRINT("测试分离邻居的鸟basespeed：%f\n", (double)boid.vari.AvoidBoidsArray[0].baseSpeed);
    DEBUG_PRINT("测试分离邻居的鸟currspeed：%f\n", (double)boid.vari.AvoidBoidsArray[0].currSpeed);
    DEBUG_PRINT("测试分离邻居的鸟positon：%f\n", (double)boid.vari.AvoidBoidsArray[0].position.x);
    DEBUG_PRINT("测试分离邻居的鸟positon：%f\n", (double)boid.vari.AvoidBoidsArray[0].position.y);
    DEBUG_PRINT("测试分离邻居的鸟positon：%f\n", (double)boid.vari.AvoidBoidsArray[0].position.z);
     
    DEBUG_PRINT("测试对齐邻居的鸟id：%c\n", boid.vari.allignmentBoidsArray[0].id);
    DEBUG_PRINT("测试对齐邻居的鸟basespeed：%d\n", boid.vari.allignmentBoidsArray[0].baseRotationSpeed);
    DEBUG_PRINT("测试对齐邻居的鸟currspeed：%d\n", boid.vari.allignmentBoidsArray[0].currRotaltionSpeed);
    DEBUG_PRINT("测试对齐邻居的鸟basespeed：%f\n", (double)boid.vari.allignmentBoidsArray[0].baseSpeed);
    DEBUG_PRINT("测试对齐邻居的鸟currspeed：%f\n", (double)boid.vari.allignmentBoidsArray[0].currSpeed);
    DEBUG_PRINT("测试对齐邻居的鸟positon：%f\n", (double)boid.vari.allignmentBoidsArray[0].position.x);
    DEBUG_PRINT("测试对齐邻居的鸟positon：%f\n", (double)boid.vari.allignmentBoidsArray[0].position.y);
    DEBUG_PRINT("测试对齐邻居的鸟positon：%f\n", (double)boid.vari.allignmentBoidsArray[0].position.z);
    
    DEBUG_PRINT("测试聚集邻居的鸟id：%c\n", boid.vari.cohesionBoidsArray[0].id);
    DEBUG_PRINT("测试聚集邻居的鸟basespeed：%d\n", boid.vari.cohesionBoidsArray[0].baseRotationSpeed);
    DEBUG_PRINT("测试聚集邻居的鸟currspeed：%d\n", boid.vari.cohesionBoidsArray[0].currRotaltionSpeed);
    DEBUG_PRINT("测试聚集邻居的鸟basespeed：%f\n", (double)boid.vari.cohesionBoidsArray[0].baseSpeed);
    DEBUG_PRINT("测试聚集邻居的鸟currspeed：%f\n", (double)boid.vari.cohesionBoidsArray[0].currSpeed);
    DEBUG_PRINT("测试聚集邻居的鸟positon：%f\n", (double)boid.vari.cohesionBoidsArray[0].position.x);
    DEBUG_PRINT("测试聚集邻居的鸟positon：%f\n", (double)boid.vari.cohesionBoidsArray[0].position.y);
    DEBUG_PRINT("测试聚集邻居的鸟positon：%f\n", (double)boid.vari.cohesionBoidsArray[0].position.z);
    

    if(boid.vari.avoidNumber==0&&boid.vari.allignmentNumber==0&&boid.vari.cohesionNumber==0){
        vTaskDelay(M2T(1000)); // 假设在发送数据包后等待1秒
    }
    }
}


void p2pcallbackHandler(P2PPacket *p)
{
 //序列复制
    static struct Base_Info copyBase;
    memcpy(&copyBase,&p->data[0], sizeof(struct Base_Info)+1);
    
    //数据呈现
    DEBUG_PRINT("打印接收的数据包\n");
    DEBUG_PRINT("飞机的id是:%c\n",copyBase.id);

    DEBUG_PRINT("x坐标是%f\n",(double)copyBase.position.x);
    
    DEBUG_PRINT("y坐标是%f\n",(double)copyBase.position.y);
    
    DEBUG_PRINT("z坐标是%f\n",(double)copyBase.position.z);

    updateBoidQueue(copyBase);
}

void appMain()
{
   DEBUG_PRINT("Waiting for activation ...\n");

    

    //初始化并拷贝
    InitBoid(&boid);
     
    //初始化数据包的端口
    p_reply.port=0x00; //声明数据包端口


    // DEBUG_PRINT("测试初始化的鸟：\n");
    // DEBUG_PRINT("测试初始化的鸟id：%c\n", boid.base.id);
    // DEBUG_PRINT("测试初始化的鸟basespeed：%d\n", boid.base.baseRotationSpeed);
    // DEBUG_PRINT("测试初始化的鸟currspeed：%d\n", boid.base.currRotaltionSpeed);
    // DEBUG_PRINT("测试初始化的鸟basespeed：%f\n", (double)boid.base.baseSpeed);
    // DEBUG_PRINT("测试初始化的鸟currspeed：%f\n", (double)boid.base.currSpeed);
    // DEBUG_PRINT("测试初始化的鸟positon：%f\n", (double)boid.base.position.x);
    // DEBUG_PRINT("测试初始化的鸟positon：%f\n", (double)boid.base.position.y);
    // DEBUG_PRINT("测试初始化的鸟positon：%f\n", (double)boid.base.position.z);
    
    
    memcpy(&p_reply.data,&boid.base,sizeof(struct Base_Info));
     
    p_reply.size=sizeof(struct Base_Info)+1; 
    // Register the callback function so that the CF can receive packets as well.
    p2pRegisterCB(p2pcallbackHandler);
    
    //将整个广播体系独立起来，相当于鸟类的声波器官



    xTaskCreate(sendDataPacketTask, "send_data_packet_task", RADIO_STACK_SIZE, NULL, RADIO_MAIN_PRI, NULL);
    xTaskCreate(divideBoidQueue, "divide_boid_queue", DIVIDE_STACK_SIZE, NULL, DIVIDE_MAIN_PRI, NULL);
    
}


//现在问题应该是传递时候的信号量问题