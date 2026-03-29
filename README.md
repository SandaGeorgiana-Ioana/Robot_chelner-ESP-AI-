# Proiect în curs de dezvoltare
Vreau să mai fac un roboțel, să adaug tăvi la ei și să fac PCB pentru circuit.
# Detalii despre proiect
Sistemul automatizează servirea într-un restaurant prin conectarea unei aplicații mobile direct la un robot de livrare. Clientul plasează comanda pe telefon, platforma software o procesează, iar robotul, dotat cu un modul SuperMini ESP32, primește instrucțiunile wireless și pleacă spre masă. Întregul proces se bazează pe protocolul MQTT, care asigură o comunicare rapidă și stabilă între aplicație și robot, transformând livrarea preparatelor într-un flux simplu, automatizat.

# Componenete principale
Aplicația mobilă (React Native / Expo) permite clientului să selecteze masa, să navigheze prin meniu organizat pe categorii, să primească recomandări personalizate generate de un model AI și să trimită comanda direct la bucătărie.


Sistemul de recomandări AI (Python / Flask / Groq API) analizează profilul clientului — preferințe alimentare, alergii, afecțiuni medicale — și generează recomandări personalizate din cele 350 de preparate disponibile, folosind un algoritm de filtrare bazat pe conținut combinat cu explicații generate de modelul de limbaj Llama 3.


Serverul central (Node.js / Express / Socket.IO) gestionează comunicarea în timp real între toate componentele sistemului, stochează comenzile într-o bază de date SQLite și coordonează trimiterea instrucțiunilor către roboți prin protocolul MQTT.


Dashboard-ul bucătăriei (React) oferă personalului o interfață tip Kanban cu patru coloane — Nouă, Preparare, Gata, Livrat — unde comenzile progresează în timp real, iar la finalizarea preparării bucătarul selectează robotul disponibil și trimite comanda de livrare.


Roboții de livrare (ESP32 / MQTT / Mosquitto) primesc instrucțiunile wireless, execută traseul predefinit către masa specificată și confirmă sosirea înapoi către server, care marchează automat comanda ca livrată.

# Demonstrație video 

https://github.com/user-attachments/assets/aed57150-3011-46e2-8f3d-2db543737dd9

<h2>Poze demonstrative</h2>

<p align="center">
  <img src="https://github.com/user-attachments/assets/2ad5aa7e-dc0b-4636-9339-46c81631aedd" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/af414ec8-4e15-4025-b5cd-917883936a56" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/41c2a2d9-c93f-4cff-b914-7c78f5bfc95b" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/5442cf31-d618-4ae5-9e21-46f96842a0aa" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/c530bc1b-55bf-467e-a4fd-499fe7bf8e6b" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/dcd5ad1d-348c-4c4c-8b3d-a65d31f59509" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/17cc34a2-8fcd-4454-b49d-661b90e28076" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/d44dd0de-b8f3-419c-93d9-3448cf18ca98" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/89b855f7-732e-40a5-a4e0-3014f4322b32" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/70a34a13-1175-481f-9d6f-6aada19c1b28" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/d29476a2-6e91-4eb4-bbfc-57a9d1648570" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/60231fbf-acd7-43e3-98d2-5c65e57da1a3" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/91cb61e9-dcd9-4fa4-a43e-73b499a3af68" width="500"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/8b0db78c-ca4c-48a9-8fe3-ca52ccbdb492" width="500"/>
</p>


