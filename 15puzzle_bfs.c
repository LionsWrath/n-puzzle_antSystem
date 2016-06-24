#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "uthash.h"

#define BASE_PHEROMONE 100.0
#define INITIAL_Q 10.0

//Pensar nas seguintes estruturas:
//  -Solução - matrix formiga*solucao
//  -Dados 
//  -Feromônios - Matrix formiga*feromonios
//  -Informação Heurística - Matrix formiga*feromonios
//  -Probabilidades
//  -Componentes Candidatos
//
//  -> Como Achar Componentes Candidatos

typedef struct table_t {
    char id[33];
    UT_hash_handle hh;
} table_t;

typedef struct _Node {
    int table[4][4];
    struct _Node * father; //father
    struct _Node * leafs[4];
    int height; //altura da arvore
    int numberOfLeafs;
    double pheromone;
    double heuristic;
} Node;

typedef struct _Member {
    struct _Node * node;
    struct _Member * next;
} Member;

typedef struct _Session {
    float alpha;
    float beta;
    float rho;
    int antNumber;
    int endTable[4][4];
    table_t *tables;
} Session;

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
    printf("ADD  Key: %s'\n",tableData->id);

    HASH_ADD_STR(tables, id, tableData);
}

bool isTableInHash(int table[][4], table_t * tables) {
    char key[33];
    struct table_t * s;
    
    key[0] = '\0';
    generateKey(table, key);
    printf("FIND Key: %s'\n",key);
    
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

    printf("Generated leaf: \n");
    printMatrix(leaf->table);

    if (!isTableInHash(leaf->table, session->tables)) {
        addTableToHash(leaf->table, session->tables);

        printf("------ADDED\n\n");

        leaf->height = node->height + 1;
        leaf->father = node;
        leaf->pheromone = BASE_PHEROMONE;
        leaf->numberOfLeafs = 0;
        leaf->heuristic = outOfOrder(session, leaf);

        node->leafs[node->numberOfLeafs++] = leaf;
    }  else {
        free(leaf);
        printf("------REFUSED\n\n");

    }
} 

//Falta gerar hash e adionar na estrutura para verificação
void generateLeafs(Session * session, Node * node) {
    int i, j;

    printf("Generating leafs\n");

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
        printf("%f %f %f \n", uppers[i], pow(leaf->pheromone, session->alpha), pow(leaf->heuristic, session->beta));
    }

    for (i = 0; i < node->numberOfLeafs; i++) {
        probability[i] = uppers[i]/lower;
        printf("Leaf: %d - %f \n", i, probability[i]);
    }
    
    for (i=0; i<4; i++) {
        chosen[i] = getMax(node, probability);
        probability[chosen[i]] = -1;
    }


    printf("Chosen0: %d\n", chosen[0]);
    printf("Chosen1: %d\n", chosen[1]);
    printf("Chosen2: %d\n", chosen[2]);
    printf("Chosen3: %d\n", chosen[3]);
    printf("-----------------------------------------------------\n");
}

int updatePheromone(float rho, Node * node) {
    int pathSize = node->height;

    do {
        node->pheromone = node->pheromone*(1.0f - rho) + INITIAL_Q/pathSize;
        node = node->father;
    } while (node != NULL);
}

void createList(Member ** member) {
    *member = (Member *)malloc(sizeof(Member));
}

void addList(Member * member, Node * node) {
    
    while (member->next) { //Until NULL
        member = member->next;
    }

    member->node = node;
    createList(&(member->next));

    member = member->next;
    member->node = NULL;
    member->next = NULL;
}

//void removeList(Member ** member, Node ** node) {
//    *node = *(member).node;
//    Member * member = *(member).next;
//    free(*member);
//    *(member) = member;
//}

//Node * removeList(Member * member) {
//    Node * node = member->node;
//}

bool applyConstruction(Session * session, Node * node) {
    int chosen[4], i=0;

    //if (member == NULL) {
    //    createList(&member);
    //}

    if (!isFinal(session->endTable, node->table)) {
        printf("Altura: %d\n", node->height);
        if (0 == node->numberOfLeafs) generateLeafs(session, node);
        if (0 == node->numberOfLeafs) return false;
        printf("NOL: %d\n", node->numberOfLeafs);

        //verify put order
        calculateProbability(session, node, chosen);
        while (!applyConstruction(session, node->leafs[chosen[i++]])) {
            if (i == 4 || chosen[i] == -1) return false;
        } 
        
        return true;
    } else {
        updatePheromone(session->rho, node);
        printf("NICE ");
        return true;
    }
}

void buildSolutions(Session session, Node root) {
    int i;
    
    for (i=0; i<session.antNumber; i++) {
        if (applyConstruction(&session, &root)) printf("RESULTADO\n");
        else printf("NOP\n");
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
    int oc, cycles;

    srand(time(NULL));

    Session session;
    session.tables = NULL;

    //Entrada
    int initialTable[4][4];
    int dummyTable[4][4] = { { 1,  2,  3,  4}, 
                             { 5,  6,  7,  8}, 
                             { 9, 10, 11, 12}, 
                             {13, 14, 15,  0} };

    memcpy(session.endTable, dummyTable, 4*4*sizeof(int));

    //Falta: 
    while ((oc = getopt(argc, argv, "a:b:r:f:n:c:")) != -1) {
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
        }
    }

    readFile(filename, initialTable);
    printMatrix(initialTable);

    Node root;

    //printf("\n%lu\n", sizeof(root));
    initializeNode(&root, initialTable);

    //Add initial table to Hash
    //struct table_t * tableData = (table_t *) malloc(sizeof(table_t));
    //tableData->id[0] = '\0';
    
    //generateKey(initialTable, tableData->id);

    //HASH_ADD_STR(session.tables, id, tableData);

    //Begin ant system
    //int i;
    //for (i=0; i<cycles; i++) 
    //    buildSolutions(session, root);

    //Clean Table
    //table_t * s, *tmp;
    //HASH_ITER(hh, session.tables, s, tmp) {
    //    printf("FREE: %s\n", s->id);
    //    HASH_DEL(session.tables, s);
    //    free(s);
    //}

    Node outro;
    initializeNode(&outro, session.endTable);

    Member * member;

    createList(&member);
    member->node = NULL;
    member->next = NULL;
    int i;
    for (i=0; i<10; i++)
        if (0 == i%2) addList(member, &outro);
        else addList(member, &root);

    //Remover e passar para o próximo
    root = *(member->node);
    Member * temp = member->next;
    free(member);
    member = temp;

    i = 0;
    while (member->next) {
        printf("%d\n", i++);
        printMatrix(member->node->table);
        member = member->next;
    }

    printMatrix(root.table);

    return 0;
}
