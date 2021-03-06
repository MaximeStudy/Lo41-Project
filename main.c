/**********************************************
* Fichier principal							  *
* Gestion du flux ferroviaire aux abords 	  *
* d'une gare								  *
* Projet LO41 - Printemps 2016                *
* Auteurs :                                   *
* Elise Kocik et Florian Bishop               *
**********************************************/

/*Librairies nécessaires à l’exécution du programme*/
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "train.c"
#include "voie.c"

int *stop;

Voie *voieA, *voieB, *voieC, *voieD, *aig1, *aig2, *gTGV, *gM1, *gM2, *gGL,*tunnel;
/* Création des mutex : un pour le tunnel et les voies de garages*/ 
/*un pour les voies C, D et l'aiguillage 2*/ 
/*un pour les voies A, B et l'aiguillage1*/
pthread_mutex_t tunGar = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t voieGL = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t marchandises = PTHREAD_MUTEX_INITIALIZER; 
#include "aiguilleur.c"
#include "trajets.c"

#define NB_TRAINS 20


/**********************************************
* Fonction de greve au début du programme     *
**********************************************/
void greve()
{
	sleep(4);
	printf("\nSuite à une grève du personnel, aucun train ne circulera aujourd'hui.\n");
	printf("\nVeuillez nous excuser pour la gène occasionnée.\n\n");
	sleep (7);
	printf("\nLa SNCF ne contrôle pas ce programme : reprise normale du trafic.\n\n");
	sleep(4);
	system("clear");
	printf("\n");
}

/**********************************************
* Initialisation des voies du réseau          *
**********************************************/
void initReseau()
{
	//printf("voieA");
	//fflush(stdout);
	voieA=createVoie("voieA",1);
	voieB=createVoie("voieB",1);
	voieC=createVoie("voieC",3);
	voieD=createVoie("voieD",3);
	aig1=createVoie("aig1",1);
	aig2=createVoie("aig2",1);
	gTGV=createVoie("gTGV",2);
	gM1=createVoie("gM1",1);
	gM2=createVoie("gM2",1);
	gGL=createVoie("gGL",2);
	tunnel=createVoie("tunnel",3);
}

/**********************************************
* Suppression des voies du réseau             *
**********************************************/
void deleteReseau()
{
	deleteVoie(voieA);
	deleteVoie(voieB);
	deleteVoie(voieC);
	deleteVoie(voieD);
	deleteVoie(aig1);
	deleteVoie(aig2);
	deleteVoie(gTGV);
	deleteVoie(gM1);
	deleteVoie(gM2);
	deleteVoie(gGL);
	deleteVoie(tunnel);
}

/************************************************************
* Création des trains et appel des fonctions de trajet      *
************************************************************/
void * fonc_Train(void *num)
{
	//printf("num thread %i \n",(int)num);
    //printf("de TID : %ld \n", (long) pthread_self());
    /* Création du train*/
	Train * t=randomTrain((int)num);
	/*Appel de la fonction de trajet correspondante*/
	switch(t->type){
		case TGV:
					if (t->direction==EO){TGV_EO(t);}
					else{TGV_OE(t);}
					break;
		case GL:
					if (t->direction==EO){GL_EO(t);}
					else{GL_OE(t);}
					break;
		case M:
					if (t->direction==EO){M_EO(t);}
					else{M_OE(t);}
					break;
		default:
					printf("Default case");
					break;
	}
	/*Suppression du train avant la fin du thread*/
	deleteTrain(t);
	/*Fin du thread*/
	pthread_exit(NULL);
}

/**********************************************
* Fonction principale du programme            *
**********************************************/
int main(int argc, char* argv[])
{
	/*La SNCF est en greve*/
	greve();
	/*Initialisation du réseau*/
	initReseau();
	srand(time(NULL));
	/* nombre de trains déf en paramètre ou via la constante*/
	int NbTrains = (argc > 1 ? atoi(argv[1]) : NB_TRAINS); 
	/*Entier permettant l'arret des threads d'aiguillage*/
	stop=(int *)malloc(sizeof(int)) ; 
	*stop=0;
	
	//printf("nbtrains : %i \n",NbTrains);
	/*NumTrain : Numéro du prochain train créé*/
	int NumTrain =0;
	/*depTrain : Nombre de trains générés dès le lancement du programme*/
	int depTrain=(int)NbTrains/5;
	/*Tableau des threads trains*/
	pthread_t tid[NbTrains];
	/*Tableau des threads aiguilleurs*/
	pthread_t aiguil[3];
	//getchar();
	int rc,k;
	/* Génération des trains dès le lancement du programme*/
	for (int tr=0; tr<depTrain;tr++){
		if(rc=pthread_create(&(tid[NumTrain]), NULL, fonc_Train, (void*)NumTrain)!=0){
	        	printf("Erreur dans la creation du thread %i",NumTrain);
				return EXIT_FAILURE;
	    }
	    else{
	    		NumTrain++;
	    }
	}
	/*Generation des aiguilleurs*/

	if(rc=pthread_create(&(aiguil[0]), NULL, fonc_P0, (void*)0)!=0){
	    printf("Erreur dans la creation du thread %i",NumTrain);
		return EXIT_FAILURE;
	}
	if(rc=pthread_create(&(aiguil[1]), NULL, fonc_P1, (void*)1)!=0){
	    printf("Erreur dans la creation du thread %i",NumTrain);
		return EXIT_FAILURE;
	}
	if(rc=pthread_create(&(aiguil[2]), NULL, fonc_P2, (void*)2)!=0){
	    printf("Erreur dans la creation du thread %i",NumTrain);
		return EXIT_FAILURE;
	}
	

	/*Génération du reste des trains à des intervalles de temps aléatoires*/
	while(NumTrain<NbTrains){
		/*Aléatoire pour espacer la création des trains*/
		k=rand()%7;
		/*Création des threads trains*/
		if(k<2){
	        if(rc=pthread_create(&(tid[NumTrain]), NULL, fonc_Train, (void*)NumTrain)!=0){
	        	printf("Erreur dans la creation du thread %i",NumTrain);
				return EXIT_FAILURE;
	    	}
	    	else{
	    		NumTrain++;
	    	}
	    }
	    /*Petit temps d'attente*/
	    usleep(200000);
    }
    /*Temps d'attente après la création de la totalité des trains*/
    sleep((int)NbTrains*0.9);
    printf("\n Attention : Etat d'urgence - Fermeture de la gare imminente! - Evacuation des trains\n\n");
    /*On attend la fin des threads trains*/
    int i;
   	for(i=0;i<NbTrains;i ++){
   		pthread_join(tid[i],NULL);
   	}
   	*stop=1;
   	//printf("Join trains OK\n");
   	/*On attend la fin des threads aiguilleurs*/
   	int j;
   	for(j=0;j<3;j++){
   		pthread_join(aiguil[j],NULL);
   	}
   	//printf("Join aiguil OK\n");
   	/*On supprime le réseau*/
   	deleteReseau();
   	//printf("Delete reseau OK\n");
   	printf("\n\n Fermeture de la gare pour cause d'état d'urgence!\n\n");
   	pthread_exit(NULL);
   	free(stop);
   	return(0);
}