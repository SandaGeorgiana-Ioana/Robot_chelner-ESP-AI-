import paho.mqtt.client as mqtt
import time
import os

BROKER_IP = "localhost"
BROKER_PORT = 1883
TOPIC_COMENZI = "restaurant/comenzi"
TOPIC_RASPUNSURI = "restaurant/raspunsuri"

def incarca_mese(fisier="mese.txt"):
    mese = {}
    try:
        with open(fisier, "r") as f:
            for linie in f:
                linie = linie.strip()
                if "=" in linie:
                    nume, cod = linie.split("=", 1)
                    mese[nume.strip()] = cod.strip()
    except FileNotFoundError:
        print(f"Fisierul {fisier} nu a fost gasit!")
    return mese

def clear():
    os.system('cls' if os.name == 'nt' else 'clear')

def afiseaza_meniu(mese):
    latime = 39
    print("╔" + "═" * latime + "╗")
    print("║" + "   Restaurant Robot — Panou Control  ".center(latime) + "║")
    print("╠" + "═" * latime + "╣")
    for i, (nume, cod) in enumerate(mese.items(), start=1):
        linie = f"  {i}  →  {nume} ({cod})"
        print("║" + linie.ljust(latime) + "║")
    print("╠" + "═" * latime + "╣")
    print("║" + "  s  →  robot stai".ljust(latime) + "║")
    print("║" + "  st →  robot status".ljust(latime) + "║")
    print("║" + "  c  →  comanda personalizata".ljust(latime) + "║")
    print("║" + "  r  →  reincarca mese.txt".ljust(latime) + "║")
    print("║" + "  q  →  iesire".ljust(latime) + "║")
    print("╚" + "═" * latime + "╝\n")

def construieste_comenzi(mese):
    comenzi = {}
    for i, (nume, cod) in enumerate(mese.items(), start=1):
        nr_masa = nume.replace("masa", "").strip()
        comenzi[str(i)] = f"robot du-te la masa {nr_masa} cod {cod}"
    comenzi["s"]  = "robot stai"
    comenzi["st"] = "robot status"
    return comenzi

def on_message(client, userdata, message):
    raspuns = message.payload.decode("utf-8")
    if "am ajuns" in raspuns or "m-am oprit" in raspuns:
        clear()
        afiseaza_meniu(userdata["mese"])
    print(f"  [RASPUNS ROBOT] → {raspuns}\n")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conectat la broker Mosquitto!\n")
        client.subscribe(TOPIC_RASPUNSURI)
    else:
        print(f"Eroare conectare, cod: {rc}")

mese = incarca_mese()

client = mqtt.Client(client_id="laptop-controller", userdata={"mese": mese})
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER_IP, BROKER_PORT)
client.loop_start()
time.sleep(1)

afiseaza_meniu(mese)
comenzi = construieste_comenzi(mese)

while True:
    alegere = input("Comanda ta: ").strip().lower()

    if alegere == "q":
        print("La revedere!")
        break

    elif alegere == "r":
        mese = incarca_mese()
        client.user_data_set({"mese": mese})
        comenzi = construieste_comenzi(mese)
        clear()
        afiseaza_meniu(mese)
        print("  [INFO] Mese reincarcate!\n")

    elif alegere == "c":
        comanda = input("Scrie comanda: ").strip()
        if comanda:
            client.publish(TOPIC_COMENZI, comanda)
            print(f"  [TRIMIS] → {comanda}\n")

    elif alegere in comenzi:
        comanda = comenzi[alegere]
        client.publish(TOPIC_COMENZI, comanda)
        print(f"  [TRIMIS] → {comanda}\n")

    else:
        print("  Optiune necunoscuta. Incearca din nou.\n")

    time.sleep(0.5)

client.loop_stop()
client.disconnect()