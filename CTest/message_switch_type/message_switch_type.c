#include <stdio.h>
#include <string.h>

int main() {
    float originalFloat = 3.14159; // 你想要转换的float值
    char floatChar[sizeof(float)]; // 创建一个与float大小相同的char数组

    // 将float转换为char数组
    memcpy(floatChar, &originalFloat, sizeof(float));

    // 将char数组还原为float
    float restoredFloat;
    memcpy(&restoredFloat, floatChar, sizeof(float));

    printf("Original float: %f\n", originalFloat);
    printf("Restored float: %f\n", restoredFloat);

    return 0;
}
