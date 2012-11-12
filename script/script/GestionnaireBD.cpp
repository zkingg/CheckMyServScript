#include "GestionnaireBD.h"

GestionnaireBD::GestionnaireBD(){
	mutex = true;//init de la mutex
	mysql_init(&mysql);
	mysql_options(&mysql,MYSQL_READ_DEFAULT_GROUP,"option");
}

void GestionnaireBD::lock(){
	while(1){
		if(mutex){//si mutex dispo
			mutex = false;//on la lock
			return;
		}
		Sleep(500);
	}
}

void GestionnaireBD::unlock(){
	mutex = true;//libérer la mutex
}

/*insertion en bd
*@param srv : info du serveur
*@param actif : résulat du test de connection au serveur
*@param info : information du fichier yml
*/
void GestionnaireBD::insertionBD(Serveur srv,bool actif, Info info){
	 if(info.mdp == "\'\'" || info.mdp == "\"\"")//si "" ou '' dans ficher yml
		 info.mdp = "";
	 if(mysql_real_connect(&mysql,info.host.c_str(),info.user.c_str(),info.mdp.c_str(),info.db.c_str(),0,NULL,0))
     {
		string etat = actif ? "1" : "0" ; // bool to string 0/1
		string query = "select idserver, etat, datedebut from server join serverupdate using(idserver) where adresse='"+srv.ip+"' and ports='"+to_string(srv.port)+"' order by datedebut desc limit 1";
		MYSQL_RES *result = NULL;
        MYSQL_ROW row;

		mysql_query(&mysql, query.c_str());//exec req : recuperation idserver,etat,datedebut
		result = mysql_store_result(&mysql);
		my_ulonglong length = mysql_affected_rows(&mysql);//nb result
		if( length == 0 ){//si premier check du serv
			query = "insert into serverupdate(idserver,datedebut,etat) values("+srv.id+",now(),"+etat+")";
			mysql_query(&mysql,query.c_str());//ajout en db
			if( mysql_affected_rows(&mysql) != 1)
				cout << "Echec MAJ" << endl;
			return;
		}
		while ((row = mysql_fetch_row(result)))
        {
			int id_server = (int)row[0];//idserver
			string datedebut = (string)row[2];//datedebut
			if( row[1] != etat ){//etat : s'il y a eu un chg d'état
				query = "update serverupdate set datefin=now() where idserver="+srv.id+" and datedebut='"+datedebut+"'";
				mysql_query(&mysql, query.c_str());//on reseigne datefin
				if( mysql_affected_rows(&mysql) != 1)
					cout << "Echec MAJ" << endl;
				else{
					query = "insert into serverupdate(idserver,datedebut,etat) values("+srv.id+",now(),"+etat+")";
					//cout<< query <<endl;
					mysql_query(&mysql,query.c_str());//ajout d'une nouvelle ligne en db
					if( mysql_affected_rows(&mysql) != 1)
						cout << "Echec MAJ" << endl;
				}
			}
        }
		mysql_free_result(result);      
    }
    else
    {
        cout<< "Une erreur s'est produite lors de la connexion à la BDD!" << endl;
	}
}

 int GestionnaireBD::getListeServeurs(vector<Serveur>* liste_srv, Info info){
	 if(info.mdp == "\'\'" || info.mdp == "\"\"")
		 info.mdp = "";
	if(mysql_real_connect(&mysql,info.host.c_str(),info.user.c_str(),info.mdp.c_str(),info.db.c_str(),0,NULL,0))
    {
		string query = "select adresse, ports,idserver from server ";
		MYSQL_RES *result = NULL;
        MYSQL_ROW row;

		mysql_query(&mysql, query.c_str());
		result =mysql_store_result(&mysql);
		//result = mysql_use_result(&mysql);
		my_ulonglong length = mysql_affected_rows(&mysql);//nb result
		Serveur* srvs = new Serveur[length];
		for(int i=0; row = mysql_fetch_row(result); i++){
			srvs[i].ip = row[0];//ip
			srvs[i].port = atol(((string)row[1]).c_str());//port
			srvs[i].id = row[2];//id
			liste_srv->push_back(srvs[i]);
		}
		mysql_free_result(result);    
		return length;
    }
    else
    {
        printf("Une erreur s'est produite lors de la connexion à la BDD!\r\n");
		return NULL;
    }
}
 //long to string
string GestionnaireBD::to_string(long val)
{
	char var[20] ;
	ltoa(val,var,10);
    return string(var);
}

//int to string
string GestionnaireBD::to_string(int val)
{
	char var[20];
	itoa(val,var,10);
    return string(var);
}

//testative de communication avec srv
bool GestionnaireBD::testServer(long port, string ip){
	// --- Initialise the use of "Ws2_32.dll"
	time_t debut = time(NULL);
	time_t fin;
	int codeRetour =0;
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2,2),&WSAData) != 0)
	{
		return false;
	}

	// --- Create a socket that is bound to a specific service provider
	SOCKET Socket = socket(AF_INET,SOCK_STREAM,0);
	if (Socket == INVALID_SOCKET)
	{
		WSACleanup();
		return false;
	}

	HOSTENT* Host = gethostbyname(ip.c_str());//recup' adresse ip
	try{
		if (Host)
		{
			SOCKADDR_IN SocketIn;
			SocketIn.sin_family           = AF_INET;//ipv4			
			SocketIn.sin_port             = htons(port);//port 
			SocketIn.sin_addr.S_un.S_addr = (DWORD)*((DWORD*)Host->h_addr_list[0]);

			int  obtTempsAttente = 0 ;
			unsigned int sizeObtTempsAttente = sizeof(obtTempsAttente);
			obtTempsAttente = 500;//millisec : ne marche pas ... mais empeche l'attente infini

			int iResult = setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO ,(const char*) &obtTempsAttente, sizeObtTempsAttente);
			if (iResult == SOCKET_ERROR) {//si nok
				wprintf(L"getsockopt for SO_KEEPALIVE failed with error: %u\n", WSAGetLastError());
				exit(0);
			}
			if (connect(Socket,(SOCKADDR*)&SocketIn,sizeof(SOCKADDR_IN)) == 0) //connexion
			{  
				char reply [255];
				char packet[12];

				memset(packet,0,12);
				packet[0] = 0x05;//s5
				packet[1] = 0x01;//no mdp & ident                           
				send( Socket, packet, 3, 0 );//envoi de la trame
				int byteRcv = recv( Socket, reply, 255, 0 );//reception de la réponse
				//cout<<errno<<endl;
				if(byteRcv < 0 )//si temps imparti ecoulé
					return false; 
				else//si recu 0 ou +
					return true; 
			}
			else
				return false;
		}

		// --- Release the resources
		closesocket(Socket);
		WSACleanup();
		return false;
	}catch(...)
	{ 
		closesocket(Socket);
		WSACleanup();
		return false;
		//return -11;
	}
}

//execution du thread
DWORD WINAPI ThreadStart(LPVOID lpthis)
{
	ParamsThread* info = (ParamsThread*) lpthis;
	GestionnaireBD* db = info->db;
	//cout << info->serveur->ip << ":"<< info->serveur->port<<endl;
	bool actif = db->testServer(info->serveur->port,info->serveur->ip);//test srv si up
	db->lock();//prise mutex
	db->insertionBD(*info->serveur,actif,*info->info);
	db->unlock();//libé mutex
	//cout << GetCurrentThreadId() << endl;
	
	db->retirerDeLaListeThread( GetCurrentThreadId());//on retire le thread de la collection
	free(info);
	ExitThread(0);
}

 void GestionnaireBD::lancerThread(Serveur* s, Info* info){
			DWORD tmpThreadId;
			HANDLE tmpThreadHandle;
			ParamsThread* params_thread = new ParamsThread();
			params_thread->db = this;
			params_thread->serveur = s;
			params_thread->info = info;

			//int l = liste_thread.size();
			while( liste_thread2.size() >= info->nb_proc ){//si nombre de thread simultané authorisé atteint 
				Sleep(1000);
			}
			cout << s->ip << ":"<< s->port<<endl;
			tmpThreadHandle = CreateThread(NULL, 0,ThreadStart , (LPVOID)params_thread ,0,&tmpThreadId);//lancement thread
			InfoThread info_thread ;
			info_thread.id = tmpThreadId;
			info_thread.thread = tmpThreadHandle;
			liste_thread2.push_back(info_thread);//ajout dans la liste de thread
 }

 int GestionnaireBD::getNbThread(){ return liste_thread2.size(); }

 //retirer thread avec son id
 void GestionnaireBD::retirerDeLaListeThread(unsigned long id){
	//cout<< "params:" << id << endl;
	//cout<< "avant:" << liste_thread2.size() << endl;
	for(int i=0; i < liste_thread2.size() ; i++){
		if(liste_thread2[i].id == id )
			liste_thread2.erase(liste_thread2.begin()+i);
	}
	//cout<< "apres:" << liste_thread2.size() << endl;
 }

 void GestionnaireBD::afficherListeThread(){
	cout<< "------------" << endl;
	for(int i=0; i < liste_thread2.size() ; i++){
		cout<< liste_thread2[i].id << endl;
	}
	cout<< "------------" << endl;
 }