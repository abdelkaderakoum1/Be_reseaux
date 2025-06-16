					BE RESEAU
TPs BE Reseau - 3 MIC-E : AL AKOUM Abdelkader .


Compilation du protocole mictcp et lancement des applications de test fournies
Pour compiler mictcp et générer les exécutables des applications de test depuis le code source fourni, 

make et Usage: ./tsock_texte [-p|-s destination] port

informations générales : 
    Toutes les versions ont été implementées sauf la version  v4.2 Le code est fonctionnel.  




CHOIX D’IMPLÉMENTATION

Gestion de la fiabilité partielle avec Stop & Wait et fenêtre glissante.

Pour assurer une fiabilité partielle du protocole,
j'ai implémenté une gestion des pertes basée sur une fenêtre glissante de taille fixe (ici, 10 éléments).
Au départ, tous les éléments de la fenêtre sont initialisés à 0,
indiquant un scénario pessimiste (aucun paquet délivré initialement).

À chaque transmission (mic_tcp_send()), un paquet est envoyé
et la réception de l'acquittement (ACK) est vérifiée.
Si l'ACK est reçu correctement, la case correspondante dans la fenêtre est notée à 0 (paquet reçu),
sinon elle passe à 1 (paquet perdu).

Ensuite, je calcule le taux de pertes actuel grâce à la fonction fenetretaux_fautRentre().
Si le taux de pertes dépasse le seuil négocié lors de l'établissement de la connexion,
une retransmission du paquet est immédiatement déclenchée.

Ce mécanisme permet une tolérance contrôlée aux pertes,
en s'assurant que le taux réel de pertes ne dépasse jamais un seuil acceptable fixé au départ.

NÉGOCIATION DU TAUX DE PERTES

Lors du début de la connexion entre le client et le serveur,
j'ai mis en place une négociation pour décider du taux de pertes maximal accepté par les deux parties.

Quand le client envoie son premier message SYN,
il indique le taux de pertes maximal qu'il peut accepter.

Le serveur reçoit cette valeur, la compare avec la sienne,
et choisit le taux le plus strict pour être sûr que les deux côtés soient satisfaits.

Ce taux négocié est ensuite renvoyé au client dans le message SYN-ACK.
Cela permet d'adapter facilement la transmission en fonction des conditions du réseau et ce rate_accepted est donner apres dans la fonction fenetretaux_fautRentre() alors que ca chnage 
de valeur toujours.

GESTION DES ÉVÉNEMENTS CÔTÉ SERVEUR (ASYNCHRONE)

Côté serveur, j'ai utilisé une gestion asynchrone,
c'est-à-dire que le serveur réagit différemment selon l'état dans lequel il se trouve (socket1.state).

Par exemple, lorsqu'il reçoit un message SYN,
il répond directement par un SYN-ACK et attend ensuite le message ACK du client pour confirmer la connexion.

De même, pour fermer proprement la connexion,
il utilise un échange précis de messages FIN et FIN-ACK.

Cette façon de faire permet au serveur de gérer facilement les événements réseau
et d'assurer une connexion stable.



note:

dansle dernier commit on ajouter un retransmission de pdu_ack comme ca si ack est perdu pour un rate loss >=50 on renvoit pour etablir la connexion