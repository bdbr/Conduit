#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h> 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "conduct.h"


struct conduct *conduct_create(const char *name, size_t a, size_t c){
    int fd;
    pthread_mutexattr_t ma;
    pthread_condattr_t ca;
    
    struct conduct * cdt;
    
    pthread_mutexattr_init(&ma);
    pthread_mutexattr_setpshared(&ma,PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&ca);
    pthread_condattr_setpshared(&ca,PTHREAD_PROCESS_SHARED);
    
    if(a < 0 || c < 0){ 
        
        fprintf(stderr,"taille ou capacite inferieur a zero\n");
        return NULL;
    }
    if(a > c){
        fprintf(stderr,"a est inferieur a c\n");
        
        
    }
    
    
    if(name != NULL){
        if((fd = open(name, O_RDWR |O_CREAT  |O_TRUNC,0666))<0){
            perror("open ");
            return NULL;
        }
        if (ftruncate(fd,c) == -1){
            perror("ftruncate ");
            exit(EXIT_FAILURE);
        }
        
        if((cdt = mmap(0, sizeof(struct conduct),
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
            perror("mmap ");
        return NULL;
        
            }
            
            close(fd);
            
    }else{
        
        if((cdt = mmap(0, sizeof(struct conduct),
            PROT_READ | PROT_WRITE, MAP_SHARED |  MAP_ANONYMOUS , -1, 0)) == MAP_FAILED){
            perror("mmap ");
        return NULL;
        
            }
    }
    memset(cdt,0,sizeof(struct conduct));
    cdt->buffer=mmap(NULL,c,PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0);
    if (cdt->buffer==MAP_FAILED)
    {
        perror("mmap");
        return NULL;
    }
    cdt->name = name;
    cdt->a = a;
    cdt->c = c;
    
    
    if(pthread_mutex_init (&(cdt->verrou), &ma)<0){
        perror("mutex int:");
    } 
    
    
    
    if(pthread_cond_init (&(cdt->lectureOk),&ca)<0){
        perror("init cond:");
    }
    if(pthread_cond_init (&(cdt->ecritureOk),&ca)<0){
        perror("cond init: ");
    }
    
    pthread_mutexattr_destroy(&ma);
    pthread_condattr_destroy(&ca);
    
    return cdt;
    
}

struct conduct * conduct_open(const char *name){
    int fd;
    struct conduct * cdt;
    
    if((fd = open(name, O_RDWR))<0){
        perror("open ");
        exit(EXIT_FAILURE);
    }
    
    if((cdt = mmap(0, sizeof(struct conduct),
        PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED){
        perror("mmap ");
    exit(EXIT_FAILURE);
    
        }
        
        close(fd);
        
        return cdt;
}

int getFreeSpace(struct conduct * c){
    if(c->fin > c->debut )
        return c->c - (c->fin - c->debut);
    else
        return c->debut - c->fin;
    
}

int getDataSize(struct conduct * c){
    if(c->fin > c->debut)
        return c->fin - c->debut;
    else
        return c->c - (c->debut - c->fin);
}


ssize_t conduct_write(struct conduct *c, const void *buf, size_t count){
    int plcLibre,dataToWritesize;
    if(count < 1)                              //Si count < 0 on ne fait rien
        return count;
    if(pthread_mutex_lock (&(c->verrou))<0){
        perror("lock write");
        exit(EXIT_FAILURE);
    }
    
    while(((c->debut==c->fin && c->nbElement>0 ) || (count > getFreeSpace(c) && count < c->a)) && c->eof==0){// si le buffer est plein
        if(pthread_cond_signal (&c->lectureOk)<0){                                                  //ou que count > a l'espace libre et count < atomicite on bloque
            perror("read broadcast:");
        }
        if(pthread_cond_wait(&(c->ecritureOk),&(c->verrou))<0){           
            
            perror("wait ecritureOk");
            exit(EXIT_FAILURE);
        }
        
    }
    
    if(c->eof==1){
        errno = EPIPE;
        if(pthread_mutex_unlock(&(c->verrou))<0){
            perror("lock write");
            exit(EXIT_FAILURE);
        }
        return -1;
    }
    plcLibre = getFreeSpace(c);
    if(count <= plcLibre|| (plcLibre==0&&c->nbElement <= 0))
        dataToWritesize = count;
    else
        dataToWritesize = (plcLibre/c->a)*c->a;
    if((c->debut < c->fin && dataToWritesize <= c->c - c->fin)||(plcLibre==0&&c->nbElement <= 0)||c->fin < c->debut){ 
        memcpy(c->buffer+c->fin,buf,dataToWritesize);
        
        
        
    }else{
        
        memcpy(c->buffer+c->fin,buf,(c->c-c->fin));
        memcpy(c->buffer,buf+(c->c-c->fin),count-(c->c-c->fin));
        
    }
    
    c->fin = (c->fin+dataToWritesize) % c->c;
    c->nbElement += dataToWritesize;
    if(pthread_cond_signal (&(c->lectureOk))<0){
        perror("broadcast lectureok:");
    }
    if(pthread_mutex_unlock (&(c->verrou))<0){
        perror("write unlock");
    }
    
    return dataToWritesize;
    
    
    return -1;
    
}


ssize_t conduct_read(struct conduct *c, void *buf, size_t count){ 
    
    int dataToReadSize;
    if(count < 1){
        return count;
        
    }
    if(pthread_mutex_lock (&(c->verrou))<0){
        perror("read lock");
        exit(EXIT_FAILURE);
    }
    
    
    while(c->debut==c->fin  && c->nbElement<=0 && c->eof==0){
        if(pthread_cond_wait(&c->lectureOk,&c->verrou)<0){        //si le buffer est vide on bloque
            perror("wait read:");
            exit(EXIT_FAILURE);
        }
        
    }
    
    if(c->eof == 1 && (c->fin==c->debut&&c->nbElement<=0)){
        if(pthread_mutex_unlock (&(c->verrou))<0){
            perror("read unlock");
            exit(EXIT_FAILURE);
        }
        return 0;
    }
    
    
    
    
    
    
    if(count >= c->nbElement)
        dataToReadSize = c->nbElement;
    else
        dataToReadSize = count;
    
    
    
    if(c->debut < c->fin ||  c->debut <= c->fin || (c->fin < c->debut && count  <= c->c-c->debut) ){
        memcpy(buf,c->buffer+c->debut,dataToReadSize);
    }else{  
        memcpy(buf,c->buffer+c->debut,c->c-c->debut);
        memcpy(buf+(c->c-c->fin),c->buffer,c->c-c->fin);
        
    }
    c->nbElement -= dataToReadSize;  
    c->debut = (c->debut+dataToReadSize) % c->c;
    if(pthread_cond_signal (&c->ecritureOk)<0){
        perror("read broadcast:");
    }
    if(pthread_mutex_unlock (&(c->verrou))<0){
        perror("read unlock:");
        exit(EXIT_FAILURE);
    }
    
    
    return dataToReadSize;
    
    
    
}

int conduct_write_eof(struct conduct *c){
    pthread_mutex_lock (&c->verrou);
    c->eof=1;
    
    if(pthread_cond_signal (&c->ecritureOk) < 0){
        perror("broadcast ecritureok eof:");
    }
    if(pthread_cond_signal (&c->lectureOk)<0){
        perror("broadcast eof lecutreok:");
    }
    
    
    if(pthread_mutex_unlock (&(c->verrou))<0){
        perror("unlock eof: ");
    }
    return 1;
    
}

void conduct_close(struct conduct *conduct){
    int size = conduct->c;
    if(munmap(conduct->buffer,size) < 0){
        perror("munmap ");
        exit(EXIT_FAILURE);
    }
    if(munmap(conduct,sizeof(struct conduct)) < 0){
        perror("munmap ");
        exit(EXIT_FAILURE);
    }
    
    
}

void conduct_destroy(struct conduct *conduct){
    const char * pathname = conduct->name;
    conduct_close(conduct);
    if(pathname != NULL){
        if(remove(pathname) <0){
            perror("remove ");
            exit(EXIT_FAILURE);
        }
    }
}

