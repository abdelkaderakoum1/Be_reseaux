#include <mictcp.h>
#include <api/mictcp_core.h>
#include <stdlib.h>
#include <stdbool.h>

#define DUP_ACK_COUNT 2
/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */

#define MAX_RETRANSMISSION 30
#define TIMER 100
#define taille_fenetre 10
#define max_loss 1
mic_tcp_sock socket1;
unsigned int numseq = 0;
unsigned int numack = 0;   

float taux_perte = 0.0;

static int idx = 0;
bool ack_received = false;
static int fenetre[taille_fenetre];

// Pour négociation dynamique du seuil
int rate_accepted=20;
int seuil_ = 10;      // valeur par défaut souhaitée côté application
int seuil_negocie_ = 20;    // résultat de la négociation, utilisé pour la fiabilité partielle


#define seuil 0
#define SEUIL_NEGOCIE 10

// Pour suivre l'état du handshake
bool waiting_synack = false;
bool waiting_finack = false;


//ici on initialise la fenetre de perte pour etre dand le cas pessimiste 
void init_fenetre() {
    for (int i = 0; i < taille_fenetre; i++) {
        fenetre[i] = 0;
    }
    idx = 0;
}

void updateFenetre(bool delivre){
    if (delivre) {
        fenetre[idx] = 0; 
    } else {
        fenetre[idx] = 1; 
    }
    idx = (idx + 1) % taille_fenetre; 
}

bool fenetretaux_fautRentre() {
    int somme = 0;
    for (int i = 0; i < taille_fenetre; i++) {
        somme += fenetre[i];
    }
    int taux = (float)somme / taille_fenetre *100; // thinking about putting %100 so when 100 
    printf("Taux de livraison dans la fenetre: %d\n", taux);
    for (int i = 0; i < taille_fenetre; i++) {
        printf("fenetre[%d]: %d\n", i, fenetre[i]);
    }
    printf("le taux accepte: %d\n",rate_accepted);
    if (taux > rate_accepted) {// s
        return true; 
    } else {
        return false; 
    }
}
    








int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(70); /* Pour simuler une perte de 50% des paquets */
   socket1.fd=1;

   socket1.local_addr.ip_addr.addr = "localhost";
    socket1.local_addr.ip_addr.addr_size = strlen(socket1.local_addr.ip_addr.addr);
    socket1.local_addr.port = 9000;
   socket1.state=IDLE;


   result= socket1.fd;
   init_fenetre();


   return result ;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   
    if (socket1.fd == socket)
    {
         socket1.local_addr = addr;
         return 0;
    }
    else
    {
         printf("[MIC-TCP] Erreur: couldn't bind le socket .\n");
         return -1;
    }
   
   return -1;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)

{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    

    if (socket1.fd != socket) return -1;

    socket1.state = SYN_RECEIVED; // Le serveur attend un SYN


    while (socket1.state != ESTABLISHED) { //on attend de changer de etat
        
    }

    if (addr) {
        *addr = socket1.remote_addr;
    }
    return 0;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */

 // Stocker l’adresse et le port de destination passés par addr dans la structure 
 //mictcp_socket correspondant au socket identifié par socket passé en paramètre.
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    
    if (socket1.fd != socket) {
        printf("Erreur: mauvais descripteur de socket.\n");
        return -1;
    }

    socket1.remote_addr = addr;
    socket1.state = SYN_SENT; 
    
    int retransmissions = 0;
    int status = -1;

    // Préparer le PDU SYN avec le taux de pertes souhaité (
     mic_tcp_pdu syn_pdu = {0}, synack_pdu = {0}, ack_pdu = {0};

    synack_pdu.payload.size = sizeof(int);
    synack_pdu.payload.data = malloc(synack_pdu.payload.size);
    syn_pdu.header.source_port = socket1.local_addr.port;
    syn_pdu.header.dest_port = addr.port;
    syn_pdu.header.syn = 1;
    syn_pdu.header.ack = 0;

    syn_pdu.header.fin = 0;
    syn_pdu.payload.size = sizeof(int);
    syn_pdu.payload.data = malloc(sizeof(int));

    int seuil_souhaite_client = seuil; // global
    memcpy(syn_pdu.payload.data, &seuil_souhaite_client, sizeof(int));

    mic_tcp_ip_addr local_addr = {0}, remote_addr = {0};
    local_addr.addr = malloc(10);
    remote_addr.addr = malloc(10);



    // Boucle d'envoi du SYN et attente du SYN_ACK avec limite de retransmissions
    do {
        printf("[MIC-TCP] Envoi SYN (tentative %d, seuil=%d)\n", retransmissions+1, seuil_souhaite_client);
        IP_send(syn_pdu, addr.ip_addr);

        status = IP_recv(&synack_pdu, &local_addr, &remote_addr, 1000);

        if (status != -1 && synack_pdu.header.syn == 1 && synack_pdu.header.ack == 1) {
            printf("[MIC-TCP] SYN/ACK reçu.\n");
            
            if (synack_pdu.payload.size >= sizeof(int)) {
                int negotiated = 0;
                memcpy(&negotiated,
                       synack_pdu.payload.data,
                       sizeof(int));
                rate_accepted = negotiated;
                
            }

            // SYN/ACK reçu
            break;
        }
        retransmissions++;
    } while (retransmissions < MAX_RETRANSMISSION);




    free(syn_pdu.payload.data);

    if (retransmissions == MAX_RETRANSMISSION) {
        printf("Nombre maximum de retransmissions SYN atteint, échec de connexion.\n");
        return -1;
    }

    // Extraction du taux négocié (en binaire)
    int seuil_negocie_tmp = seuil;
    
    
    
    

    printf("[MIC-TCP] Seuil négocié obtenu: %d\n", seuil_negocie_tmp);

    // Envoi du ACK pour finaliser le handshake
    ack_pdu.header.source_port = socket1.local_addr.port;
    ack_pdu.header.dest_port = addr.port;
    ack_pdu.header.syn = 0;
    ack_pdu.header.ack = 1;
    ack_pdu.header.fin = 0;
    ack_pdu.payload.size = 0;
    ack_pdu.payload.data = NULL;
    for (int i = 0; i < DUP_ACK_COUNT; i++) {
        IP_send(ack_pdu, addr.ip_addr);
}

    socket1.state = ESTABLISHED;
    return 0;

    




}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    mic_tcp_pdu pdu;
    mic_tcp_pdu ack;
    ack.payload.data = malloc(8);
    ack.payload.size = 8;
    
	pdu.payload.data = mesg;
	pdu.payload.size = mesg_size;
    pdu.header.source_port = socket1.local_addr.port;
    pdu.header.dest_port = socket1.remote_addr.port;
    pdu.header.seq_num = numseq;
    pdu.header.ack_num = 0;
    pdu.header.syn = 0;
    pdu.header.ack = 0;
    pdu.header.fin = 0;
    //pdu.header.syn = 0;
    //pdu.header.fin= 0;
    mic_tcp_ip_addr local_addr;
    mic_tcp_ip_addr remote_addr;

    //Envoyer un message (dont la taille le contenu sont passés en paramètres).
    printf("test1\n");
    int sent_size =IP_send(pdu,socket1.remote_addr.ip_addr);
    printf("on a envoye un pdu\n");
    int recv_size = IP_recv(&ack, &local_addr, &remote_addr,100);
    if (recv_size >= 0 ||ack.header.ack == 1 || ack.header.ack_num == numseq) {
        ack_received= true;
        
       
    }
    updateFenetre(ack_received);

    ack_received= false;
    
    if (fenetretaux_fautRentre()) {
        printf("on est dans le if\n");
    while (recv_size < 0 || ack.header.ack != 1 || ack.header.ack_num != numseq){
        printf("on est dans la boucle\n");
        printf("recv_size:%d\n",recv_size);

        printf("ack:%d\n",ack.header.ack);
        printf("ack_num%d\n",ack.header.ack_num);
        printf("numseq:%d\n",numseq);
       
        sent_size =IP_send(pdu,socket1.remote_addr.ip_addr);
        
        perror("send\n");
        recv_size = IP_recv(&ack, &local_addr, &remote_addr, 100);

        if (recv_size >= 0 ||ack.header.ack == 1 || ack.header.ack_num == numseq) {
            printf("on a finally recu un ack\n");
            
        
            idx=(taille_fenetre + idx -1)%taille_fenetre;
            fenetre[idx] = 0;
            fenetretaux_fautRentre();
    }
        

        
    

        
    }
}
    
    
        perror("recv\n"); 
    
    printf("on a sortie du BOUCLE\n");
    printf("recv_size:%d\n",recv_size);

    printf("ack:%d\n",ack.header.ack);
    printf("ack_num%d\n",ack.header.ack_num);
    printf("numseq:%d\n",numseq);


    
    numseq++;


    return sent_size;


    
    

    //return -1;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    if (socket1.fd != socket)
    {
        printf("[MIC-TCP] Erreur dans le recv de socket .\n");
        return -1;
    }
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_payload payload;
	payload.data = mesg;
	payload.size = max_mesg_size;
	int effective_data_size = app_buffer_get(payload);
	return effective_data_size; // ou -1 si erreur ou pb;

    
    
    //return -1;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    
    if (socket1.fd != socket) return -1;



    if (socket1.state != ESTABLISHED && socket1.state != CLOSING) {
        printf("[MIC-TCP] Erreur : socket non connecté ou déjà fermé.\n");
        return -1;
    }

    socket1.state = CLOSING;

    // Création du PDU FIN
    mic_tcp_pdu fin_pdu = {0}, finack_pdu = {0}, ack_pdu = {0};
    fin_pdu.header.source_port = socket1.local_addr.port;
    fin_pdu.header.dest_port = socket1.remote_addr.port;
    fin_pdu.header.syn = 0;
    fin_pdu.header.ack = 0;
    fin_pdu.header.fin = 1;
    fin_pdu.payload.size = 0;
    fin_pdu.payload.data = NULL;

    // Initialisation des adresses pour IP_recv (localhost)
    mic_tcp_ip_addr local_addr = {0}, remote_addr = {0};
    local_addr.addr = "localhost";
    local_addr.addr_size = strlen(local_addr.addr);
    remote_addr.addr = "localhost";
    remote_addr.addr_size = strlen(remote_addr.addr);

    int retransmissions = 0;
    int status = -1;

    // Boucle d'envoi du FIN et attente du FIN_ACK
    do {
        printf("[MIC-TCP] Envoi FIN (tentative %d)\n", retransmissions + 1);
        IP_send(fin_pdu, socket1.remote_addr.ip_addr);

        status = IP_recv(&finack_pdu, &local_addr, &remote_addr, TIMER);

        if (status != -1 && finack_pdu.header.fin == 1 && finack_pdu.header.ack == 1) {
            break;
        }
        retransmissions++;
    } while (retransmissions < MAX_RETRANSMISSION);

    if (retransmissions == MAX_RETRANSMISSION) {
        printf("[MIC-TCP] Nombre maximum de retransmissions FIN atteint, échec fermeture.\n");
        return -1;
    }

    // Envoi du dernier ACK pour terminer proprement le handshake
    ack_pdu.header.source_port = socket1.local_addr.port;
    ack_pdu.header.dest_port = socket1.remote_addr.port;
    ack_pdu.header.syn = 0;
    ack_pdu.header.ack = 1;
    ack_pdu.header.fin = 0;
    ack_pdu.payload.size = 0;
    ack_pdu.payload.data = NULL;
    IP_send(ack_pdu, socket1.remote_addr.ip_addr);

    socket1.state = CLOSED;
    printf("[MIC-TCP] Connexion fermée proprement.\n");
    return 0;

    







}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_ip_addr local_addr, mic_tcp_ip_addr remote_addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu ack;
    //memset(&ack, 0, sizeof(mic_tcp_pdu));
    ack.header.source_port = socket1.local_addr.port;
    ack.header.dest_port = socket1.remote_addr.port;
    //ack.header.seq_num = 0;
    //ack.header.ack_num = pdu.header.seq_num; 
    //ack.header.ack = 1;
    int seuil_client;
    int seuil_negocie = 10;
    int final_rate;
    // SYN reçu (Handshake 1) 
    if (pdu.header.syn == 1 && pdu.header.ack == 0) {
        printf("[MIC-TCP] SYN reçu.\n");
        // Extraction du seuil proposé par le client (binaire)
        
        
        memcpy(&seuil_client, pdu.payload.data, sizeof(int));

        // Calcul moyenne entre client et serveur que pour le premier syn
        if (seuil_client<= seuil_negocie){
                final_rate = seuil_client;
            } else {
                final_rate = seuil_client;
            }

            rate_accepted=final_rate;


            
        
        printf("[MIC-TCP] Seuil négocié: %d\n", final_rate);
        socket1.state = SYN_RECEIVED;
        socket1.remote_addr.ip_addr = remote_addr;


        // Réponse SYN_ACK avec seuil négocié
        ack.header.syn = 1;
        ack.header.ack = 1;
        ack.payload.size = sizeof(int);
        ack.payload.data = malloc(sizeof(int));
        memcpy(ack.payload.data, &final_rate, sizeof(int));
        //socket1.state = SYN_RECEIVED;
        // Enregistrer l'adresse distante
        socket1.remote_addr.ip_addr = remote_addr;
        IP_send(ack, remote_addr);

        free(ack.payload.data);
        return;
    }

    
    

    // ACK reçu (Handshake 3) 
    if (pdu.header.ack == 1 && pdu.header.syn == 0 && socket1.state == SYN_RECEIVED) {
        socket1.state = ESTABLISHED;
        printf("[MIC-TCP] Connexion établie (ACK handshake reçu).\n");
        return;
    }

     //  FIN reçu (fermeture connexion) 
    if (pdu.header.fin == 1 && pdu.header.ack == 0) {
        // Répondre FIN_ACK
        ack.header.fin = 1;
        ack.header.ack = 1;
        IP_send(ack, remote_addr);
        if (socket1.state != CLOSING){
            socket1.state = CLOSING;}
        return;
    }
    //  FIN_ACK reçu (côté client) 
    if (pdu.header.fin == 1 && pdu.header.ack == 1 && socket1.state == CLOSING) {
        socket1.state = CLOSED;
        return;
    }

    // PDU de données 
    
 
    if (socket1.state == ESTABLISHED && pdu.header.fin == 0 && pdu.header.syn == 0) {
        
        // Traitement normal
        ack.header.ack = 1;
        ack.header.ack_num = pdu.header.seq_num;
        ack.payload.size = sizeof(int);
        ack.payload.data = malloc(sizeof(int));
        
        IP_send(ack, remote_addr);
        
        if (pdu.header.seq_num >= numack) {
            app_buffer_put(pdu.payload);
            numack++;
        }
        return;
    }
}
/*
    IP_send(ack,remote_addr);
    
    if (pdu.header.seq_num >= numack ){
        app_buffer_put(pdu.payload);
        numack++;
    }*/
    

