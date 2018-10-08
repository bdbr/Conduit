#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "conduct.h"


typedef struct demande
{
    long writerId;
    int x;
    int y;
    
}demande;

typedef struct resultat
{
    struct demande * dmd;
    long readerId;
    int res;
    
}resultat;



int fd[2];
int fd2[2];

int pgcd(int x,int y){
    int res = 0,i;
    for(i=1; i <= x && i <= x; ++i)
    {
        if(x%i==0 && y%i==0)
            res = i;
    }
    
    return res;
}





void *reader(void * arg)
{
    
    struct demande * dmd = malloc(sizeof(struct demande));
    struct resultat * res  = malloc(sizeof(struct resultat));
    
    int r =read(fd[0], dmd, sizeof(demande));
    
    if(r < 0){
        perror("reader: ");
    }
    
    printf ("Reader: Writerid:%ld x:%d y:%d\n", dmd->writerId,dmd->x,dmd->y);
    res->res = pgcd(dmd->x,dmd->y);
    res->readerId = pthread_self();
    res->dmd = dmd;

    if(write(fd2[1], res, sizeof(resultat))<0){
        perror("write");
    }

    return NULL;
    
    
}




void *writer(void * arg)
{  
    
    
    struct resultat * res  = malloc(sizeof(struct resultat));
    int x = rand() % 1000 + 1;
    int y = rand() % 1000 +1;
    struct demande dmd = {pthread_self(),x,y};
    int r = write(fd[1], &dmd, sizeof(demande));
    if(r < 0){
        perror("write: ");
    } 
 
    if(read(fd2[0],res, sizeof(resultat))<0)
        perror("read");
    printf("writer:%ld readerId:%ld x:%d y:%d pgcd:%d\n",res->dmd->writerId,res->readerId,res->dmd->x,res->dmd->y,res->res);
    return NULL;
    
    
}

int main(int argc,char * argv[])
{   
    
    srand(time(NULL));
    int nbThread = atoi(argv[1]);
    pthread_t writers [nbThread];
    pthread_t readers[nbThread];
    int  i;
   
  
    if(pipe(fd)<0){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if(pipe(fd2)<0){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    for(i=0;i<nbThread;i++){
        pthread_create(&writers[i],NULL,writer,NULL);
        
    }
    
    for(i=0;i<nbThread;i++){
        pthread_create(&readers[i],NULL,reader,NULL);
    }
    for(i = 0; i < nbThread; i++)
    {
        pthread_join(writers[i],NULL);
    }
    
    for(i = 0; i < nbThread; i++)
    {
        pthread_join(readers[i],NULL);
    }
    close(fd[0]);
    close(fd[1]);
    close(fd2[0]);
    close(fd2[1]);
    
    return 0;
    
}