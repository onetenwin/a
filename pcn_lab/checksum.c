
#include <stdio.h>

int main() {
    int n, data[20], sum = 0, carry, checksum;

    printf("Enter number of 8-bit words: ");
    scanf("%d", &n);

    printf("Enter each word in hexadecimal (00 to FF):\n");
    for (int i = 0; i < n; i++)
        scanf("%x", &data[i]);

    // Sender: 1's complement addition
    for (int i = 0; i < n; i++) {
        sum += data[i];
        carry = sum >> 8;
        sum = (sum & 0xFF) + carry;
        printf("Sum = %02X, Carry = %d\n", sum, carry);
    }

    checksum = (~sum) & 0xFF;
    printf("Checksum = %02X\n", checksum);

    // Receiver: add data and checksum
    sum = checksum;

    // making it mamual
    for (int i = 0; i < n; i++) {
        sum += data[i];
        sum = (sum & 0xFF) + (sum >> 8);
    }
    
    if (sum == 0xFF)
        printf("Receiver: No error detected\n");
    else
        printf("Receiver: Error detected\n");

    return 0;
}
