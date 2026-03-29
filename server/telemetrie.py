import paho.mqtt.client as mqtt
import time
import os

BROKER_IP = "localhost"
TOPIC_TELEMETRIE = "restaurant/telemetrie"

ultimul_mesaj = ""
timp_ultimul_mesaj = None

def clear():
    os.system('cls' if os.name == 'nt' else 'clear')

def on_message(client, userdata, message):
    global ultimul_mesaj, timp_ultimul_mesaj
    ultimul_mesaj = message.payload.decode("utf-8")
    timp_ultimul_mesaj = time.time()

client = mqtt.Client(client_id="telemetrie-monitor")
client.on_message = on_message
client.connect(BROKER_IP, 1883)
client.subscribe(TOPIC_TELEMETRIE)
client.loop_start()

print("Ascult telemetrie...\n")

while True:
    clear()
    if timp_ultimul_mesaj is not None:
        elapsed = time.time() - timp_ultimul_mesaj
        secunde = int(elapsed)
        zecimi  = int((elapsed - secunde) * 10)
        timer   = f"{secunde:02d}.{zecimi}"
        print(f"  Ultimul mesaj acum {timer}s\n")
        print(f"  {ultimul_mesaj}\n")
    else:
        print("  Astept date...\n")
    time.sleep(0.1)