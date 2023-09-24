/**
 * ,---------,       ____  _ __
 * |  ,-^-,  |      / __ )(_) /_______________ _____  ___
 * | (  O  ) |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * | / ,--´  |    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *    +------`   /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2019 Bitcraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * peer_to_peer.c - App layer application of simple demonstartion peer to peer
 *  communication. Two crazyflies need this program in order to send and receive.
 */


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

#define MESSAGE "hello world"
#define MESSAGE_LENGHT 13

struct Boid
{
    char id;
    float position_x;
    float position_y;
    float position_z;
    
};




void p2pcallbackHandler(P2PPacket *p)
{
  // Parse the data from the other crazyflie and print it
//   uint8_t other_id = p->data[0];
    // static char msg[MESSAGE_LENGHT + 1];
//   memcpy(&msg, &p->data[1], sizeof(char)*MESSAGE_LENGHT);
//   msg[MESSAGE_LENGHT] = 0;
//   uint8_t rssi = p->rssi;
  
//   DEBUG_PRINT("[RSSI: -%d dBm] Message from CF nr. %d, %s\n", rssi, other_id, msg);

    // 输出数据包的内容
    DEBUG_PRINT("Data:\n");
    for (int i = 0; i < p->size; i++) {
        DEBUG_PRINT("%02X ", p->data[i]);
        if ((i + 1) % 16 == 0) {
            DEBUG_PRINT("\n");
        }
    }


    struct Boid copyBoid;
    memcpy(&copyBoid,&p->data[0], sizeof(struct Boid));
    
    DEBUG_PRINT("Deserialized CopyBoid:\n");
    DEBUG_PRINT("COPY_ID: %c\n", copyBoid.id);
    DEBUG_PRINT("COPY_Position_x: %f\n", (double)copyBoid.position_x);
    DEBUG_PRINT("COPY_Position_y: %f\n", (double)copyBoid.position_y);
    DEBUG_PRINT("COPY_Position_z: %f\n", (double)copyBoid.position_z);
}

void appMain()
{
  while(1) {

    struct Boid boid;
    boid.id = 'A';


    logVarId_t idX = logGetVarId("stateEstimate", "x");
    float X = 0.0f;
    X = logGetFloat(idX);
    boid.position_x = (double)X;
    
    logVarId_t idY = logGetVarId("stateEstimate", "y");
    float Y = 0.0f;
    Y = logGetFloat(idY);
    boid.position_y = (double)Y;
    
    logVarId_t idZ = logGetVarId("stateEstimate", "z");
    float Z = 0.0f;
    Z = logGetFloat(idZ);
    boid.position_z = (double)Z;
    

    // 计算需要的char数组大小
    size_t serializedSize = sizeof(struct Boid);
    
    // 创建一个char数组，用于存储序列化后的数据
    char serializedData[serializedSize];

    // 将结构体boid序列化到char数组
    memcpy(serializedData, &boid, serializedSize);

    // 现在你可以使用serializedData进行传输或保存

    // 示例：将char数组反序列化为结构体
    struct Boid deserializedBoid;
    memcpy(&deserializedBoid, serializedData, serializedSize);

    DEBUG_PRINT("Deserialized Boid:\n");
    DEBUG_PRINT("ID: %c\n", deserializedBoid.id);
    DEBUG_PRINT("Position_x: %f\n", (double)deserializedBoid.position_x);
    DEBUG_PRINT("Position_y: %f\n", (double)deserializedBoid.position_y);
    DEBUG_PRINT("Position_z: %f\n", (double)deserializedBoid.position_z);
    
    // Send a message every 2 seconds
    //   Note: if they are sending at the exact same time, there will be message collisions, 
    //    however since they are sending every 2 seconds, and they are not started up at the same
    //    time and their internal clocks are different, there is not really something to worry about
    
    DEBUG_PRINT("Waiting for activation ...\n");

    // Initialize the p2p packet 
    static P2PPacket p_reply;
    p_reply.port=0x00;


    // 将结构体boid序列化到数据包的char数组
    memcpy(&p_reply.data, serializedData, serializedSize);
    p_reply.size=serializedSize+1;

    // Register the callback function so that the CF can receive packets as well.
    p2pRegisterCB(p2pcallbackHandler);

    vTaskDelay(M2T(2000));
    radiolinkSendP2PPacketBroadcast(&p_reply);
  }
}