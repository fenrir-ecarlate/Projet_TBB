#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#define NB_ASCII_CHARS (256) /* Taille de la table ASCII (facilite l'indexage)*/
#define NB_OCC_CHARS    62 /* Nombre de caractère dont l'occurence doit être vérifié*/

#define MAX_BUFFER 256
#define NB_UPPER_CASE 26
#define NB_LOWER_CASE 26
#define NB_NUMBERS    10

#define NB_THREAD 4

struct char_stats {
    char *path;
    uint32_t size;
    char tab_occ[NB_ASCII_CHARS];
};

struct thread_data{
    char *path;
    uint32_t start; /* position de départ du bloc à traiter*/
    size_t size;
};

void* char_count(void* args) {
    
    size_t nb_read;
    uint8_t buffer[MAX_BUFFER];
    uint8_t *occ;
    struct thread_data *data = (struct thread_data *) args;
    
    FILE *fp = fopen(data->path, "r");
    if (fp == NULL) {
        pthread_exit(NULL);
    }
    
    occ = calloc(NB_ASCII_CHARS, sizeof *occ);
    
    fseek(fp, data->start, SEEK_SET);
    
    // Trouve le minimum entre buffer et taille à lire (bits trick)
    size_t size_to_read = data->size ^ ((MAX_BUFFER ^ data->size) & -(MAX_BUFFER < data->size));
    
    /* Traite le bloc de mémoire assigné */
    while((nb_read = fread(buffer, 1, size_to_read, fp)) != 0){
        for(size_t i=0; i < nb_read; i++){
            occ[buffer[i]]++;
        }
        
        // Trouve le minimum entre buffer et taille restent à lire (bits trick)
        // Si on arrive à la fin, size_to_read vaudra 0
        data->size -= size_to_read;
        size_to_read = data->size ^ ((MAX_BUFFER ^ data->size) & -(MAX_BUFFER < data->size));
    }
    
    fclose(fp);
    
    pthread_exit(occ);
}

void print_occurences(struct char_stats *stats){
    for(int i = 0; i < NB_UPPER_CASE; i++){
        printf("%c : %u\n", 'a' + i, stats->tab_occ['a' + i]);
        printf("%c : %u\n", 'A' + i, stats->tab_occ['A' + i]);
    }
    
    for(int i = 0; i < NB_NUMBERS; i++){
        printf("%c : %u\n", '0' + i, stats->tab_occ['0' + i]);
    }
}

struct char_stats *stats_init(const char *path)
{
    FILE *fp = fopen(path, "r");
    
    /* Vérifie que le fichier existe */
    if(fp == NULL){
        return NULL;
    }
    
    /* Retrouve la taille du fichier */
    fseek (fp , 0 , SEEK_END);
    size_t size = ftell (fp);
    
    fclose(fp);
    
    struct char_stats *stats;
    
    /* Alloue de l'espace pour la structure à rendre*/
    stats = calloc(1,sizeof *stats);
    /* Vérifie que la mémoire a bien été alloué*/
    if(stats == NULL){
        return NULL;
    }
    
    stats->size = size;
    size_t lenght = strlen(path) + 1;
    stats->path = malloc(lenght);
    memcpy(stats->path, path, lenght);
    
    return stats;
    
}

size_t stats_count(struct char_stats *stats)
{
    assert(stats != NULL);
    size_t size_per_thread;
    size_t rest;
    
    pthread_t threads[NB_THREAD];
    struct thread_data datas[NB_THREAD];
    char *occ_thread;
    
    memset(stats->tab_occ, 0, NB_ASCII_CHARS);
    
    size_per_thread = stats->size / NB_THREAD;
    rest = stats->size % NB_THREAD;
    
    /* Prépare les threads avec les bloc mémoire à traiter */
    datas[NB_THREAD-1].path = stats->path;
    datas[NB_THREAD-1].start = (NB_THREAD-1) * size_per_thread + rest;
    datas[NB_THREAD-1].size = size_per_thread;
    pthread_create(&threads[NB_THREAD-1], NULL, char_count, &datas[NB_THREAD-1]);
    
    for(int i = NB_THREAD-2; i >= 0; i--){
        datas[i].path = stats->path;
        datas[i].start = i * size_per_thread;
        datas[i].size = size_per_thread;
        pthread_create(&threads[i], NULL, char_count, &datas[i]);
    }
    
    for(int i = 0; i < NB_THREAD; i++){
        pthread_join(threads[i], (void**)&occ_thread);
        
        // Vérifie que le pointeur retourné est non null
        if(occ_thread == NULL) continue;
        
        // Met à jour le tableau des occurences
        for(unsigned char j = 'a'; j <= 'z'; j++){
            stats->tab_occ[j] += occ_thread[j];
        }
        
        for(unsigned char j = 'A'; j<= 'Z'; j++){
            stats->tab_occ[j] += occ_thread[j];
        }
        
        for(unsigned char j = '0'; j <= '9'; j++){
            stats->tab_occ[j] += occ_thread[j];
        }
        free(occ_thread);
    }
    return NB_OCC_CHARS;
}

void stats_clear(struct char_stats *stats)
{
    free(stats->path);
    free(stats);
}

int main(int argc, const char* argv[]){

    struct char_stats *stats;
    uint32_t nb_count;
   
    if(argc < 2){
        printf("parameter: <filepath>\n");
        exit(1);
    }
    
    if((stats = stats_init(argv[1])) == NULL){
        printf("Fail at initialisation\n");
        exit(1);
    }
    
    stats_count(stats);
    
    print_occurences(stats);
        
    stats_clear(stats);
     
    return 0;
}