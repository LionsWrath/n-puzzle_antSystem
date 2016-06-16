#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
//#include <misc.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

//Pensar nas seguintes estruturas:
//  -Solução - matrix formiga*solucao
//  -Dados 
//  -Feromônios - Matrix formiga*feromonios
//  -Informação Heurística - Matrix formiga*feromonios
//  -Probabilidades
//  -Componentes Candidatos
//
//  -> Como Achar Componentes Candidatos

typedef struct {
    int table[4][4];
    struct Node * father; //father
    struct Node * leafs[4];
    int height; //altura da arvore
    int pheromone; 
} Node;

typedef struct {
    float alpha;
    float beta;
    float rho;
    int antNumber;
    int endTable[4][4];
} Session;

void printMatrix(int matrix[][4]) {
    int i, j;
    for (i=0; i<4; i++) {
        for (j=0; j<4; j++)
            printf("%d ", matrix[i][j]);
        printf("\n");
    }
}

void readFile(char filename[], int initialTable[][4]) {
    FILE * input;
    size_t len = 1;
    ssize_t read;
    char *line = NULL;
    int matrix[4][4];
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

/*
//Falta gerar hash e adionar na estrutura para verificação
void generateLeafs(Node * node) {
    int i, j;
    int qtdLeafs = 0;

    for (i=0; i<4; i++) {
        for (j=0; j<4; j++) {
            if (0 == node->table[i][j]) {
                if (i-1 >= 0) {
                    Node * leaf = malloc(sizeof(Node));
                    
                    memcpy(node->table, leaf->table, 4*4*sizeof(int));
                    leaf->height = node->height + 1;
                    leaf->father = node;

                    leaf->table[i][j] = leaf->table[i-1][j];
                    leaf->table[i-1][j] = 0;

                    node->leafs[qtdLeafs] = leaf;
                    qtdLeafs++;
                }
                if (i+1 <= 3) {
                    Node * leaf = malloc(sizeof(Node));
                    
                    memcpy(node->table, leaf->table, 4*4*sizeof(int));
                    leaf->height = node->height + 1;
                    leaf->father = node;

                    leaf->table[i][j] = leaf->table[i+1][j];
                    leaf->table[i+1][j] = 0;

                    node->leafs[qtdLeafs] = leaf;
                    qtdLeafs++;
                }
                if (j-1 >= 0) {
                    Node * leaf = malloc(sizeof(Node));
                    
                    memcpy(node->table, leaf->table, 4*4*sizeof(int));
                    leaf->height = node->height + 1;
                    leaf->father = node;

                    leaf->table[i][j] = leaf->table[i][j-1];
                    leaf->table[i][j-1] = 0;

                    node->leafs[qtdLeafs] = leaf;
                    qtdLeafs++;
                }
                if (j+1 <= 3) {
                    Node * leaf = malloc(sizeof(Node));
                    
                    memcpy(node->table, leaf->table, 4*4*sizeof(int));
                    leaf->height = node->height + 1;
                    leaf->father = node;

                    leaf->table[i][j] = leaf->table[i][j+1];
                    leaf->table[i][j+1] = 0;

                    node->leafs[qtdLeafs] = leaf;
                    qtdLeafs++;
                }
            }
        }
    }
}
*/

float heuristic(Session * session, Node * node) {
    //heuristica
    return 0.0f;
}

void calculateProbability(Session * session, Node * node) {
    int i = 0, j;
    float uppers[4];
    float lower = 0;

    float probability[4];

    Node * leaf = node->leafs[i];
    while (leaf != NULL) { //Setar todas as folhas para NULL
        uppers[i] = pow(leaf->pheromone, session->alpha) * pow(heuristic(session, leaf), session->beta);
        lower += uppers[i];
        i++;
    }

    for (j=0; j<i; j++) {
        probability[j] = uppers[i]/lower;
        printf("%f ", probability[i]);
    }
    printf("\n");
}

int evaporate(float rho) {}

int deposit() {}

void updatePheromone(float rho) {
    evaporate(rho);
    deposit();
}

void applyConstruction(float alpha, float beta) {}

int isSolution(int initialTable[][4], int endTable[][4]) {
    if (!memcmp(initialTable, endTable, 4*4*sizeof(int))) return true;
    return false;
}

void buildSolutions(Session session, int initialTable[][4]) {
    int i;
    
    for (i=0; i<session.antNumber; i++) {
        while (!isSolution(initialTable, session.endTable)) {
            applyConstruction(session.alpha, session.beta); //Probability calc
            updatePheromone(session.rho);
        }
        //Apply Local Search
        //Eliminate Redundancy
    }
}


int main(int argc, char *argv[]) {
    FILE *in;
    char filename[50];
    int oc, cycles;

    Session session;

    //Testing
    int nodeQtd = 300;
    int actualQtd = 0;
    Node * nodeGroup = malloc(nodeQtd*sizeof(Node));

    //Entrada
    int initialTable[4][4];
    int dummyTable[4][4] = { { 1,  2,  3,  4}, 
                             { 5,  6,  7,  8}, 
                             { 9, 10, 11, 12}, 
                             {13, 14, 15,  0} };
    memcpy(dummyTable, session.endTable, 4*4*sizeof(int));

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
    printMatrix(session.endTable);

    int i;
    for (i=0; i<cycles; i++) 
        buildSolutions(session, initialTable);

    //int *solution;
    //malloc_matrix_int(&solution, N, C); //setar para zero

}
