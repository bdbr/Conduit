#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
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

typedef struct conducts 
{
    struct conduct * val;
    struct conduct * res;
}conducts;

int pgcd(int x,int y){
    int res=0,i;
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
    struct conducts * c = ( struct conducts *)arg;
    
    int r = conduct_read(c->val, dmd, sizeof(demande));
    
    if(r < 0){
        perror("cond_reader: ");
    }
    
    printf ("Reader: Writerid:%ld x:%d y:%d\n", dmd->writerId,dmd->x,dmd->y);
    res->res = pgcd(dmd->x,dmd->y);
    res->readerId = pthread_self();
    res->dmd = dmd;
    if(conduct_write(c->res, res, sizeof(resultat))<0){
        perror("conduct_write");
    }
    pthread_exit(NULL);
    return NULL;
}




void *writer(void * arg)
{  
    
    struct resultat * res  = malloc(sizeof(struct resultat));
    struct conducts * c= (struct conducts *)arg;
    int x = rand() % 1000 + 1;
    int y = rand() % 1000 +1;
    struct demande dmd = {pthread_self(),x,y};
    int r = conduct_write(c->val, &dmd, sizeof(demande));
    if(r < 0){
        perror("conduct_write: ");
    } 
    // printf("writer: Writerid:%ld x:%d y:%d\n",pthread_self(),x,y);
    if(conduct_read(c->res, res, sizeof(resultat))<0){
        perror("conduct_read");
    }
    
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
    struct conduct * cdt;
    struct conduct * reponse;
    struct conducts * c;
     
    c = malloc(sizeof(struct conducts));
    cdt = conduct_create(NULL, sizeof(demande),
                         sizeof(demande)*3);
    
    reponse = conduct_create(NULL, sizeof(resultat),
                             sizeof(resultat)*3);
    c->val = cdt;
    c->res = reponse;
    for(i=0;i<nbThread;i++){
        pthread_create(&writers[i],NULL,writer,(void*) c);
        
    }
    
    for(i=0;i<nbThread;i++){
        pthread_create(&readers[i],NULL,reader,(void*)c);
    }
    for(i = 0; i < nbThread; i++)
    {
        pthread_join(writers[i],NULL);
    }
    
    for(i = 0; i < nbThread; i++)
    {
        pthread_join(readers[i],NULL);
    }
    conduct_destroy(c->val);
    conduct_destroy(c->res);
    return 0;
    
}