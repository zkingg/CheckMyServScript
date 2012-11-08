
#include "GestionnaireBD.h";
//recupérer information du fichier yml
void getNbProcess(Info* info){
	int nb_proc = 0;

	std::ifstream file;
	file.open("script.conf.yml", std::ios::out);
	if(! file.is_open())//si fichier non crée
	{
		file.close();
		cout << "Creation du fichier de conf" <<endl;
		
		ofstream objetfichier; 
		objetfichier.open("script.conf.yml", ios::out);
		objetfichier << "CONF :"<< endl;
		objetfichier << "  NbProcessus : 10"<< endl;
		objetfichier << "  Run: on"<< endl;
		objetfichier << "MySQL:"<< endl;
		objetfichier << "  Host: localhost"<< endl;
		objetfichier << "  Login: root"<< endl;
		objetfichier << "  Password: ''"<< endl;
		objetfichier << "  Database: mydb"<< endl;

		objetfichier.close();
		file.open("script.conf.yml", std::ios::out);
	}

	string line = "";
	char host[50];
	char login[50];
	char mdp[50];
	char db[50];
	char run[10];
	while(getline(file,line)){
		//recup des params dans le yml
		//cout << line << endl;
		if(line.find("NbProcessus") !=string::npos)
			sscanf(line.c_str(),"  NbProcessus : %d",&info->nb_proc);
		else if(line.find("Run") !=string::npos)
			sscanf(line.c_str(),"  Run : %s",run);
		else if(line.find("Host") !=string::npos)
			sscanf(line.c_str(),"  Host : %s",host);
		else if(line.find("Login") !=string::npos)
			sscanf(line.c_str(),"  Login : %s",login);
		else if(line.find("Password") !=string::npos)
			sscanf(line.c_str(),"  Password : %s",mdp);
		else if(line.find("Database") !=string::npos)
			sscanf(line.c_str(),"  Database : %s",db);
	}
	file.close();
	info->host = host;
	info->user = login;
	info->mdp = mdp;
	info->db = db;
	info->run = run;
}

void main(){
	Info info ;
	getNbProcess(&info);
	GestionnaireBD* db = new GestionnaireBD();
	vector<Serveur> liste_srv;
	db->getListeServeurs(&liste_srv,info);//recup liste srv a check
	do{
		for (int i =0; i < liste_srv.size() ; i++){
			//cout<< liste_srv[i].ip <<","<<liste_srv[i].port<<endl;
			//todo in new process
			db->lancerThread(&liste_srv[i],&info);
		}
		//db->afficherListeThread();
		while(db->getNbThread() != 0){//attente que tout les thread ai fini leur execution
			Sleep(2000);
		}
		getNbProcess(&info);//actualisation des info du fichier yml
		cout << "Cycle fini" << endl;
	}while(info.run != "off" );//tant que run n'est pas a off dans le fichier yml
	free(db);
	system("pause");
}