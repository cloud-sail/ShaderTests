#pragma once


#define DEFINE_TOPN_INDICES_FUNC(NAME, ARRAY_SIZE, TOPN) \
void NAME(float data[ARRAY_SIZE], out int indices[TOPN]) \
{ \
    float temp[ARRAY_SIZE]; \
    int idx[ARRAY_SIZE]; \
    [unroll] \
    for (int i = 0; i < ARRAY_SIZE; ++i) { \
        temp[i] = data[i]; \
        idx[i] = i; \
    } \
    [unroll] \
    for (int i = 1; i < ARRAY_SIZE; ++i) { \
        float key = temp[i]; \
        int keyIdx = idx[i]; \
        int j = i - 1; \
        while (j >= 0 && temp[j] < key) { \
            temp[j + 1] = temp[j]; \
            idx[j + 1] = idx[j]; \
            j--; \
        } \
        temp[j + 1] = key; \
        idx[j + 1] = keyIdx; \
    } \
    [unroll] \
    for (int i = 0; i < TOPN; ++i) \
        indices[i] = idx[i]; \
}


/***Use case***
DEFINE_TOPN_INDICES_FUNC(Top4Indices16, 16, 4)

float myData[16] = { ... };
int topIndices[4];
Top4Indices16(myData, topIndices); 
*/

#define DEFINE_TOPN_INDICES_UNORDERED_FUNC(NAME, ARRAY_SIZE, TOPN) \
void NAME(float data[ARRAY_SIZE], out int indices[TOPN]) \
{ \
    float topValues[TOPN]; \
    int topIndices[TOPN]; \
    [unroll] \
    for (int i = 0; i < TOPN; ++i) { \
        topValues[i] = data[i]; \
        topIndices[i] = i; \
    } \
    [unroll] \
    for (int i = TOPN; i < ARRAY_SIZE; ++i) { \
        float minValue = topValues[0]; \
        int minIndex = 0; \
        [unroll] \
        for (int j = 1; j < TOPN; ++j) { \
            if (topValues[j] < minValue) { \
                minValue = topValues[j]; \
                minIndex = j; \
            } \
        } \
        if (data[i] > minValue) { \
            topValues[minIndex] = data[i]; \
            topIndices[minIndex] = i; \
        } \
    } \
    [unroll] \
    for (int i = 0; i < TOPN; ++i) \
        indices[i] = topIndices[i]; \
}

/*
DEFINE_TOPN_INDICES_UNORDERED_FUNC(Top3IndicesUnordered16, 16, 3)

float myData[16] = { ... };
int topIndices[3];
Top3IndicesUnordered16(myData, topIndices);

If the arraysize is determined in Runtime, use actual size as input and rewrite the function above
*/


