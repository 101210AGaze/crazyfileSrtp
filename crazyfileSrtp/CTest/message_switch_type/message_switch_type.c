#include <stdio.h>
#include <string.h>

int main() {
    float originalFloat = 3.14159; // 你想要转换的 float 值
    char floatChar[sizeof(float)]; // 创建一个与 float 大小相同的 char 数组

    // 将 float 转换为 char 数组
    memcpy(floatChar, &originalFloat, sizeof(float));

    printf("Float Char Sequence (Hexadecimal):\n");
    for (size_t i = 0; i < sizeof(float); i++) {
        printf("%02X ", (unsigned char)floatChar[i]);
    }
    printf("\n");

    // 将 char 数组还原为 float
    float restoredFloat;
    memcpy(&restoredFloat, floatChar, sizeof(float));

    printf("Original float: %f\n", originalFloat);
    printf("Restored float: %f\n", restoredFloat);

    return 0;
}
