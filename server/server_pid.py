# server_pid.py
import paho.mqtt.client as mqtt
import os

BROKER_IP        = "localhost"
TOPIC_PID        = "restaurant/pid"
TOPIC_COMENZI    = "restaurant/comenzi"
TOPIC_RASPUNSURI = "restaurant/raspunsuri"
TOPIC_TELEMETRIE = "restaurant/telemetrie"

kp, ki, kd = 0.0, 0.0, 0.0
base_speed  = 70
min_speed   = 0
max_speed   = 255
ultimul_raspuns = ""

def afiseaza_meniu():
    os.system('cls' if os.name == 'nt' else 'clear')
    print("=" * 40)
    print("       PID TEST SERVER")
    print("=" * 40)
    print(f"  KP         = {kp}")
    print(f"  KI         = {ki}")
    print(f"  KD         = {kd}")
    print("-" * 40)
    print(f"  BASE_SPEED = {base_speed}")
    print(f"  MIN_SPEED  = {min_speed}")
    print(f"  MAX_SPEED  = {max_speed}")
    print("-" * 40)
    if ultimul_raspuns:
        print(f"  Robot: {ultimul_raspuns}")
    print("-" * 40)
    print("  p X    - seteaza KP")
    print("  i X    - seteaza KI")
    print("  d X    - seteaza KD")
    print("  bs X   - seteaza BASE_SPEED")
    print("  mins X - seteaza MIN_SPEED")
    print("  maxs X - seteaza MAX_SPEED")
    print("  start  - porneste robotul")
    print("  stai   - opreste robotul")
    print("  exit   - iese din program")
    print("=" * 40)
    print("> ", end="", flush=True)

def on_message(client, userdata, message):
    global ultimul_raspuns
    if message.topic == TOPIC_RASPUNSURI:
        ultimul_raspuns = message.payload.decode("utf-8")
        afiseaza_meniu()

client = mqtt.Client(client_id="server-pid")
client.on_message = on_message
client.connect(BROKER_IP, 1883)
client.subscribe(TOPIC_RASPUNSURI)
client.loop_start()

afiseaza_meniu()

COMENZI_VALIDE = ('p', 'i', 'd', 'bs', 'mins', 'maxs')

while True:
    cmd = input("").strip()

    if cmd == "exit":
        break

    elif cmd == "start":
        client.publish(TOPIC_COMENZI, "robot start")
        afiseaza_meniu()

    elif cmd == "stai":
        client.publish(TOPIC_COMENZI, "robot stai")
        afiseaza_meniu()

    elif cmd.split()[0] in COMENZI_VALIDE if cmd.split() else False:
        parts = cmd.split()
        if len(parts) != 2:
            print("Format: <comanda> <valoare>")
            print("> ", end="", flush=True)
            continue
        try:
            tip = parts[0]
            val = float(parts[1])

            if   tip == 'p':    kp         = val
            elif tip == 'i':    ki         = val
            elif tip == 'd':    kd         = val
            elif tip == 'bs':   base_speed = int(val)
            elif tip == 'mins': min_speed  = int(val)
            elif tip == 'maxs': max_speed  = int(val)

            client.publish(TOPIC_PID, cmd)
            afiseaza_meniu()

        except ValueError:
            print("Valoare invalida")
            print("> ", end="", flush=True)

    else:
        print("Comanda necunoscuta")
        print("> ", end="", flush=True)

client.loop_stop()
client.disconnect()