#include <stdio.h>
#include <stdlib.h>

struct Boid {
    char id;
    float x;
    float y;
    float z;
};

int main() {
    struct Boid* boidArray = NULL;
    int numBoids = 0;

    // 模拟添加 Boid，与你的代码相同
    for (char id = 'A'; id <= 'D'; id++) {
        numBoids++;
        boidArray = (struct Boid*)realloc(boidArray, numBoids * sizeof(struct Boid));

        if (boidArray == NULL) {
            fprintf(stderr, "内存分配失败\n");
            exit(1);
        }

        struct Boid newBoid;
        newBoid.id = id;
        newBoid.x = 1.0 * numBoids;
        newBoid.y = 2.0 * numBoids;
        newBoid.z = 3.0 * numBoids;

        boidArray[numBoids - 1] = newBoid;
    }

    // 打印数组中的所有 Boid
    for (int i = 0; i < numBoids; i++) {
        printf("Boid %c: Position (x, y, z): (%f, %f, %f)\n", boidArray[i].id, boidArray[i].x, boidArray[i].y, boidArray[i].z);
    }

    // 要删除的 Boid 标识符
    char idToDelete = 'C';

    // 创建新的结构体数组（比原始数组小 1）
    struct Boid* newBoidArray = (struct Boid*)malloc((numBoids - 1) * sizeof(struct Boid));
    if (newBoidArray == NULL) {
        fprintf(stderr, "内存分配失败\n");
        exit(1);
    }

    int newIndex = 0;

    // 复制除了要删除的 Boid 之外的所有元素到新数组中
    for (int i = 0; i < numBoids; i++) {
        if (boidArray[i].id != idToDelete) {
            newBoidArray[newIndex] = boidArray[i];
            newIndex++;
        }
    }

    // 释放原始数组的内存
    free(boidArray);

    // 更新数组的大小和包含的 Boid 数量
    numBoids--;

    // 指向新的数组
    boidArray = newBoidArray;

    // 打印删除后的数组中的所有 Boid
    for (int i = 0; i < numBoids; i++) {
        printf("Boid %c: Position (x, y, z): (%f, %f, %f)\n", boidArray[i].id, boidArray[i].x, boidArray[i].y, boidArray[i].z);
    }

    // 释放新的数组的内存
    free(boidArray);

    return 0;
}
