#include <stdio.h>

void sender();
void receiver(int data[], int n, int checksum);

void sender()
{
    int data[] = {10, 20, 30};
    int n = 3, i, sum = 0, checksum;

    for (i = 0; i < n; i++)
        sum += data[i];

    // Wrap around carry for 8-bit checksum
    while (sum > 255)
        sum = (sum & 255) + (sum >> 8);

    checksum = (~sum) & 255;

    printf("Integer Data: ");
    for (i = 0; i < n; i++)
        printf("%d ", data[i]);

    printf("\nChecksum: %d\n", checksum);

    receiver(data, n, checksum);
}

void receiver(int data[], int n, int checksum)
{
    int i, sum = checksum;

    for (i = 0; i < n; i++)
        sum += data[i];

    while (sum > 255)
        sum = (sum & 255) + (sum >> 8);

    if (sum == 255)
        printf("Receiver: No Error Detected\n");
    else
        printf("Receiver: Error Detected\n");
}

int main()
{
    sender();
    return 0;
}