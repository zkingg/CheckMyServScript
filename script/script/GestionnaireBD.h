
#ifndef __GESTIONNAIRE_BD__
	#define  __GESTIONNAIRE_BD__
	#include <map>
	#include <string>
	#include <iostream>
	#include <fstream>
	#include <vector>
	#include <time.h>
	#include <winsock2.h>
	#pragma comment(lib, "ws2_32.lib")
	#include <MYSQL/mysql.h>
	using namespace std;
#endif

struct Info{
	string host;
	string user;
	string mdp;
	string db;
	int nb_proc;
	string run;
};

struct Serveur {
		string id; 
		 string ip;
		 long port;
};

struct InfoThread{
	DWORD id;
	HANDLE thread;
};

 class GestionnaireBD{	 
 private:
	 bool mutex;
	 MYSQL mysql; 
	 map<DWORD, HANDLE> liste_thread;//doit être présent sinon bug ...
	 vector<InfoThread> liste_thread2;
 public :
	 void lock();
	 void unlock();
	 GestionnaireBD();
	 ~GestionnaireBD() {mysql_close(&mysql);}
	 void insertionBD(Serveur srv,bool actif,Info info);
	 string to_string (long val);
	 string to_string (int val);
	 int getListeServeurs(vector<Serveur>*,Info info);
	 bool testServer(long port, string ip);
	 void lancerThread(Serveur* s, Info* info);
	 int getNbThread();
	 void retirerDeLaListeThread(unsigned long id);
	 void afficherListeThread();
};


 struct ParamsThread {
	Info* info;
	Serveur* serveur;
	GestionnaireBD* db;
};