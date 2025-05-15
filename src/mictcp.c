#include <mictcp.h>
#include <api/mictcp_core.h>

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
mic_tcp_sock socket1;


int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(0);
   socket1.fd=1;
   socket1.state=IDLE;
   result= socket1.fd;


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
    

    if (socket1.fd == socket)
    {
        socket1.state = ESTABLISHED;
        
        
        return 0;
    }
    else
    {
        printf("[MIC-TCP] Erreur dans l accept de socket .\n");
        return -1;
    }
    return -1;
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
    
    if (socket1.fd == socket)
    {
        socket1.remote_addr = addr;
       
        socket1.state = ESTABLISHED;
        return 0;
    }
    else
    {
        printf("[MIC-TCP] Erreur dans le connect de socket .\n");
        return -1;
    }   
    
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    mic_tcp_pdu pdu;
	pdu.payload.data = mesg;
	pdu.payload.size = mesg_size;
    pdu.header.source_port = socket1.local_addr.port;
    pdu.header.dest_port = socket1.remote_addr.port;
    //Envoyer un message (dont la taille le contenu sont passés en paramètres).
    int sent_size =IP_send(pdu,socket1.remote_addr.ip_addr);

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
    
    if (socket1.fd == socket)
    {
        socket1.state = CLOSED;
        return 0;
    }
    else
    {
        printf("[MIC-TCP] Erreur dans le close de socket .\n");
        return -1;
    }
    return -1;
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

   

    app_buffer_put(pdu.payload);

}
