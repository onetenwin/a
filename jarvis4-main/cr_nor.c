#include <stdio.h>
#include <string.h>

void sender();
void receiver(char frame[], char generator[]);

void sender()
{
    char data[] = "1101011011";
    char generator[] = "10011";
    char frame[100], temp[100];
    int i, j, len, g;

    len = strlen(data);
    g = strlen(generator);

    strcpy(frame, data);

    // Append zeros
    for (i = 0; i < g - 1; i++)
        frame[len + i] = '0';

    frame[len + g - 1] = '\0';

    strcpy(temp, frame);

    // CRC division
    for (i = 0; i < len; i++)
    {
        if (temp[i] == '1')
        {
            for (j = 0; j < g; j++)
            {
                if (temp[i + j] == generator[j])
                    temp[i + j] = '0';
                else
                    temp[i + j] = '1';
            }
        }
    }

    // Append CRC checksum
    for (i = 0; i < g - 1; i++)
        frame[len + i] = temp[len + i];

    frame[len + g - 1] = '\0';

    printf("Original Data: %s\n", data);
    printf("CRC Checksum: ");

    for (i = len; i < len + g - 1; i++)
        printf("%c", frame[i]);

    printf("\nTransmitted Frame: %s\n", frame);

    receiver(frame, generator);
}

void receiver(char frame[], char generator[])
{
    char temp[100];
    int i, j, len, g;

    strcpy(temp, frame);

    len = strlen(frame) - strlen(generator) + 1;
    g = strlen(generator);

    // CRC division
    for (i = 0; i < len; i++)
    {
        if (temp[i] == '1')
        {
            for (j = 0; j < g; j++)
            {
                if (temp[i + j] == generator[j])
                    temp[i + j] = '0';
                else
                    temp[i + j] = '1';
            }
        }
    }

    // Check remainder
    for (i = len; i < len + g - 1; i++)
    {
        if (temp[i] == '1')
        {
            printf("Receiver: Error Detected\n");
            return;
        }
    }

    printf("Receiver: No Error Detected\n");
}

int main()
{
    sender();
    return 0;
}