import subprocess
import sys

# ȘTERGE liniile cu ngrok.connect și conf
print("\n🚀 Server AI pornit local pe portul 5001")
print("👉 Adresa locală: http://172.20.10.2:5001\n")

# Porneste Flask (app.py)
subprocess.run([sys.executable, 'app.py'])