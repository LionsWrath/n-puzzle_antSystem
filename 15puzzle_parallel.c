#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "uthash.h"

#define MAXIMUM_HEIGHT 104000 
#define BASE_PHEROMONE 100.0
#define INITIAL_Q 10.0

pthread_mutex_t mutexsum;

typedef struct table_t {
    char id[33];
    UT_hash_handle hh;
} table_t;

typedef struct _Node {
    int table[4][4];
    struct _Node * father; 
    struct _Node * leafs[4];
    int height; 
    int numberOfLeafs;
    double pheromone;
    double heuristic;
} Node;

typedef struct _Session {
    float alpha;
    float beta;
    float rho;
    int antNumber;
    int endTable[4][4];
    table_t *tables;

    Node * best;
} Session;

typedef struct _ThreadData {
    long threadId;
    int numberOfThreads;
    
    Session * session;
    Node * node;
} ThreadData;

void printMatrix(int matrix[][4]) {
    int i, j;
    for (i=0; i<4; i++) {
        for (j=0; j<4; j++)
            printf("%d ", matrix[i][j]);
        printf("\n");
    }
    printf("\n");
}

float outOfOrder(Session * session, Node * node) {
    int out = 17, i, j;
    
    for (i=0; i<4; i++)
        for (j=0; j<4; j++)
            if (session->endTable[i][j] != node->table[i][j])
                out--;
    
    return (float)out;
}

float calculateDistance(Node * node, int i, int j) {
    int placeMap[16][2] = { {3,3}, {0,0}, {0,1}, {0,2}, 
                         {0,3}, {1,0}, {1,1}, {1,2}, 
                         {1,3}, {2,0}, {2,1}, {2,2}, 
                         {2,3}, {3,0}, {3,1}, {3,2}};

    int correctPlace[2];
    correctPlace[0] = placeMap[node->table[i][j]][0];
    correctPlace[1] = placeMap[node->table[i][j]][1];

    float dist = abs(j - correctPlace[1]) + abs(correctPlace[0] - i);

    return dist;
}

float manhattanDistance(Session * session, Node * node) {
    float totalDistance = 0;
    int i, j;
    for (i=0; i<4; i++)
        for (j=0; j<4; j++)
            if (session->endTable[i][j] != node->table[i][j])
                totalDistance += calculateDistance(node, i, j);
    
    return 100 - totalDistance;  
}

void generateKey(int table[][4], char key[]) {
    char temp[5];
    int i, j;
    for(i=0; i<4; i++) {
        for(j=0; j<4; j++) {
            if(table[i][j] < 10) strcat(key, "0");
            sprintf(temp, "%d", table[i][j]);
            strcat(key, temp);
        }
    }
}

void addTableToHash(int table[][4], table_t * tables) { 
    struct table_t * tableData = (table_t *) malloc(sizeof(table_t));
    tableData->id[0] = '\0';
    
    generateKey(table, tableData->id);
    //printf("ADD  Key: %s'\n",tableData->id);

    HASH_ADD_STR(tables, id, tableData);
}

bool isTableInHash(int table[][4], table_t * tables) {
    char key[33];
    struct table_t * s;
    
    key[0] = '\0';
    generateKey(table, key);
    //printf("FIND Key: %s'\n",key);
    
    HASH_FIND_STR(tables, key, s);

    if (s) return true;
    else return false;
}

void readFile(char filename[], int initialTable[][4]) {
    FILE * input;
    size_t len = 1;
    ssize_t read;
    char *line = NULL;
    int i, j;

    input = fopen(filename, "r");
    if (input == NULL) exit(1);

    for (i=0; i<4; i++) {
        getline(&line, &len, input);
        strtok(line, "\n");
        
        strtok(line, " ");
        for (j=0; j<4; j++) {
            initialTable[i][j] = atoi(line);
            line = strtok(NULL, " ");
        }
    }
}

void createLeaf(Session * session, Node * node, 
        int i, int j, int addi, int addj) {
    Node * leaf = malloc(sizeof(Node));
    
    memcpy(leaf->table, node->table, 4*4*sizeof(int));
    leaf->table[i][j] = leaf->table[i + addi][j + addj];
    leaf->table[i + addi][j + addj] = 0;

    //printf("Generated leaf: \n");
    //printMatrix(leaf->table);

    if (!isTableInHash(leaf->table, session->tables)) {
        addTableToHash(leaf->table, session->tables);

        //printf("------ADDED\n\n");

        leaf->height = node->height + 1;
        leaf->father = node;
        leaf->pheromone = BASE_PHEROMONE;
        leaf->numberOfLeafs = 0;
        leaf->heuristic = outOfOrder(session, leaf);

        node->leafs[node->numberOfLeafs++] = leaf;
    }  else {
        free(leaf);
        //printf("------REFUSED\n\n");

    }
} 

//Falta gerar hash e adionar na estrutura para verificação
void generateLeafs(Session * session, Node * node) {
    int i, j;

    //printf("Generating leafs\n");

    for (i=0; i<4; i++) {
        for (j=0; j<4; j++) {
            if (0 == node->table[i][j]) {
                if (i-1 >= 0) {
                    createLeaf(session, node, i, j, -1, 0);
                }
                if (i+1 <= 3) {
                    createLeaf(session, node, i, j, +1, 0);
                }
                if (j-1 >= 0) {
                    createLeaf(session, node, i, j, 0, -1);
                }
                if (j+1 <= 3) {
                    createLeaf(session, node, i, j, 0, +1);
                }
            }
        }
    }
}

int getMax(Node * node, float probability[4]) {
    int max = 0, i;
    for (i = 1; i < node->numberOfLeafs; i++) {
        if (probability[i] == probability[max]) {
            if (0 == rand()%2) {
                max = i;
            }
        }
        else if (probability[i] > probability[max]) {
            max = i;
        }
    }
    if (probability[max] == -1) return -1;
    return max; 
}

int isFinal(int endTable[][4], int initialTable[][4]) {
    if (!memcmp(initialTable, endTable, 4*4*sizeof(int))) return true;
    return false;
}

int calculateProbability(Session * session, Node * node, int chosen[4]) {
    int i = 0, j;
    float uppers[4];
    float lower = 0;

    float probability[4];

    Node * leaf;

    for (i = 0; i < node->numberOfLeafs; i++) {
        leaf = node->leafs[i];

        uppers[i]  = pow(leaf->pheromone, session->alpha); 
        uppers[i] *= pow(leaf->heuristic, session->beta);
        lower += uppers[i];
        //printf("%f %f %f \n", uppers[i], pow(leaf->pheromone, session->alpha), pow(leaf->heuristic, session->beta));
    }

    for (i = 0; i < node->numberOfLeafs; i++) {
        probability[i] = uppers[i]/lower;
        //printf("Leaf: %d - %f \n", i, probability[i]);
    }
    
    for (i=0; i<4; i++) {
        chosen[i] = getMax(node, probability);
        probability[chosen[i]] = -1;
    }


    //printf("Chosen0: %d\n", chosen[0]);
    //printf("Chosen1: %d\n", chosen[1]);
    //printf("Chosen2: %d\n", chosen[2]);
    //printf("Chosen3: %d\n", chosen[3]);
    //printf("-----------------------------------------------------\n");
}

int updatePheromone(float rho, Node * node) {
    int pathSize = node->height;

    do {
        node->pheromone = node->pheromone*(1.0f - rho) + INITIAL_Q/pathSize;
        node = node->father;
    } while (node != NULL);
}

bool applyConstruction(Session * session, Node * node) {
    int chosen[4], i=0;

    if (!isFinal(session->endTable, node->table) && node->height < MAXIMUM_HEIGHT) {
        //printf("Altura: %d\n", node->height);
        if (0 == node->numberOfLeafs) {
            pthread_mutex_lock(&mutexsum);
            generateLeafs(session, node);
            pthread_mutex_unlock(&mutexsum);
        }
        
        
        if (0 == node->numberOfLeafs) return false;
        //printf("NOL: %d\n", node->numberOfLeafs);

        calculateProbability(session, node, chosen);
        while (!applyConstruction(session, node->leafs[chosen[i++]])) {
            if (i == 4 || chosen[i] == -1) return false;
        } 
        
        return true;
    } else if (node->height >= MAXIMUM_HEIGHT) {
        pthread_mutex_lock(&mutexsum);
        updatePheromone(session->rho + .2, node);
        pthread_mutex_unlock(&mutexsum);
        return false;
    } else {
        pthread_mutex_lock(&mutexsum);
        updatePheromone(session->rho, node);
        if (session->best == NULL) {
            session->best = node;
        } else if (session->best->height > node->height) {
            session->best = node;
        }

        pthread_mutex_unlock(&mutexsum);
        return true;
    }
}

void * buildSolutions(void * threadData) {
    struct _ThreadData * TD = (struct _ThreadData *) threadData;

    long tId = (long) TD->threadId;

    int dataVision = TD->session->antNumber/TD->numberOfThreads;
    
    int i; 
    for (i=0; i<dataVision; i++) {
        applyConstruction(TD->session, TD->node);
    }
}

void initializeNode(Node * node, int matrix[][4]) {
    node->height = 0;
    node->father = NULL;
    memset(node->leafs, -1, sizeof(node)*4);
    node->pheromone = BASE_PHEROMONE;
    node->numberOfLeafs = 0;
    memcpy(node->table, matrix, 4*4*sizeof(int));
}

void findPath(Session * session, Node * root) {
    //Find best path
}

int main(int argc, char *argv[]) {
    FILE *in;
    char filename[50];
    int oc, cycles, numberOfThreads = 1;

    srand(time(NULL));

    Session session;
    session.tables = NULL;
    session.best = NULL;

    int initialTable[4][4];
    int dummyTable[4][4] = { { 1,  2,  3,  4}, 
                             { 5,  6,  7,  8}, 
                             { 9, 10, 11, 12}, 
                             {13, 14, 15,  0} };

    memcpy(session.endTable, dummyTable, 4*4*sizeof(int));

    while ((oc = getopt(argc, argv, "a:b:r:f:n:c:t:")) != -1) {
        switch(oc) {
            case 'a':
                session.alpha = atof(optarg);
                break;
            case 'b':
                session.beta = atof(optarg);
                break;
            case 'r':
                session.rho = atof(optarg);
                break;
            case 'f':
                strcpy(filename, optarg);
                break;
            case 'n':
                session.antNumber = atoi(optarg);
                break;
            case 'c':
                cycles = atoi(optarg);
                break;
            case 't':
                numberOfThreads = atoi(optarg);
                break;
        }
    }

    readFile(filename, initialTable);
    //printMatrix(initialTable);

    Node root;

    initializeNode(&root, initialTable);
    pthread_t * threads = malloc(numberOfThreads*sizeof(pthread_t));

    struct table_t * tableData = (table_t *) malloc(sizeof(table_t));
    tableData->id[0] = '\0';
    
    generateKey(initialTable, tableData->id);

    HASH_ADD_STR(session.tables, id, tableData);

    ThreadData * threadData = malloc(numberOfThreads*sizeof(ThreadData));

    //Start measuring
    struct timespec start, finish;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    int i,j;
    for (i=0; i<cycles; i++) {
        for (j=0; j<numberOfThreads; j++) {
            pthread_create(&(threads[j]), NULL, buildSolutions, (void *) threadData);
            //buildSolutions(session, root);
        }

        for (j=0; j<numberOfThreads; j++) 
            pthread_join(threads[j], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &finish);

    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Total time: %f\n\n", elapsed);

    table_t * s, *tmp;
    HASH_ITER(hh, session.tables, s, tmp) {
        //printf("FREE: %s\n", s->id);
        HASH_DEL(session.tables, s);
        free(s);
    }

    return 0;
}
