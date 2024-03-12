#pragma once

#define ARRAY_BUFFER_SIZE 8192

template <typename T>
void printArray(T A[], int size)
{
    for (int i = 0; i < size; i++)
        std::cout << A[i] << " ";
    std::cout << std::endl;
}
